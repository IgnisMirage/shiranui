#include "robot_sim/lidar_sim_component.hpp"

#include <tf2/LinearMath/Quaternion.h>
#include <tf2/utils.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

#include <cmath>
#include <limits>

namespace robot_sim
{
LidarSimNode::LidarSimNode(const rclcpp::NodeOptions & node_options)
: Node("lidar_sim", node_options),
  tf_buffer_(get_clock()),
  tf_listener_(tf_buffer_)
{
  map_frame_id_ = declare_parameter<std::string>("map_frame_id", "map");
  base_frame_id_ = declare_parameter<std::string>("base_frame_id", "base_link");
  laser_frame_id_ = declare_parameter<std::string>("laser_frame_id", "laser");
  scan_topic_ = declare_parameter<std::string>("scan_topic", "scan");
  publish_rate_ = declare_parameter<double>("publish_rate", 10.0);
  range_min_ = declare_parameter<double>("range_min", 0.05);
  range_max_ = declare_parameter<double>("range_max", 30.0);
  angle_min_ = declare_parameter<double>("angle_min", -3.141592653589793);
  angle_max_ = declare_parameter<double>("angle_max", 3.141592653589793);
  num_beams_ = declare_parameter<int>("num_beams", 720);
  angular_resolution_deg_ = declare_parameter<double>("angular_resolution_deg", 0.0);
  occupied_threshold_ = declare_parameter<int>("occupied_threshold", 50);
  laser_x_ = declare_parameter<double>("laser_x", 0.0);
  laser_y_ = declare_parameter<double>("laser_y", 0.0);
  laser_yaw_ = declare_parameter<double>("laser_yaw", 0.0);
  publish_laser_tf_ = declare_parameter<bool>("publish_laser_tf", true);
  enable_noise_ = declare_parameter<bool>("enable_noise", true);
  range_stddev_ = declare_parameter<double>("range_stddev", 0.015);
  dropout_probability_ = declare_parameter<double>("dropout_probability", 0.005);
  intensity_stddev_ = declare_parameter<double>("intensity_stddev", 8.0);
  range_resolution_ = declare_parameter<double>("range_resolution", 0.01);
  random_seed_ = declare_parameter<int>("random_seed", 0);

  if (angular_resolution_deg_ > 1e-6) {
    const double span_rad = angle_max_ - angle_min_;
    const double increment_rad = angular_resolution_deg_ * M_PI / 180.0;
    num_beams_ = static_cast<int>(std::ceil(span_rad / increment_rad)) + 1;
  }

  if (num_beams_ < 2) {
    RCLCPP_WARN(get_logger(), "num_beams must be >= 2, using 720");
    num_beams_ = 720;
  }

  if (random_seed_ != 0) {
    rng_.seed(static_cast<std::mt19937::result_type>(random_seed_));
  } else {
    std::random_device rd;
    rng_.seed(rd());
  }

  map_subscriber_ = create_subscription<nav_msgs::msg::OccupancyGrid>(
    "map", rclcpp::QoS(10).transient_local().reliable().keep_last(1),
    std::bind(&LidarSimNode::onMapReceived, this, std::placeholders::_1));

  scan_publisher_ = create_publisher<sensor_msgs::msg::LaserScan>(scan_topic_, 10);

  if (publish_laser_tf_) {
    static_tf_broadcaster_ = std::make_unique<tf2_ros::StaticTransformBroadcaster>(*this);
    publishStaticLaserTf();
  }

  const auto period = std::chrono::duration<double>(1.0 / publish_rate_);
  scan_timer_ = create_wall_timer(
    std::chrono::duration_cast<std::chrono::nanoseconds>(period),
    std::bind(&LidarSimNode::publishScan, this));

  RCLCPP_INFO(
    get_logger(),
    "lidar_sim started: %s -> %s, %d beams (%.3f deg), range_res=%.3f m @ %.1f Hz",
    laser_frame_id_.c_str(), map_frame_id_.c_str(), num_beams_,
    (angle_max_ - angle_min_) / static_cast<double>(num_beams_ - 1) * 180.0 / M_PI,
    range_resolution_, publish_rate_);
}

void LidarSimNode::publishStaticLaserTf()
{
  geometry_msgs::msg::TransformStamped tf;
  tf.header.stamp = now();
  tf.header.frame_id = base_frame_id_;
  tf.child_frame_id = laser_frame_id_;
  tf.transform.translation.x = laser_x_;
  tf.transform.translation.y = laser_y_;
  tf.transform.translation.z = 0.0;
  tf2::Quaternion q;
  q.setRPY(0.0, 0.0, laser_yaw_);
  tf.transform.rotation = tf2::toMsg(q);
  static_tf_broadcaster_->sendTransform(tf);
}

void LidarSimNode::onMapReceived(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
{
  map_ = *msg;
  map_received_ = true;
  if (!msg->header.frame_id.empty()) {
    map_frame_id_ = msg->header.frame_id;
  }
  RCLCPP_INFO(
    get_logger(), "map received: %ux%u, resolution=%.3f",
    map_.info.width, map_.info.height, map_.info.resolution);
}

bool LidarSimNode::mapReceived() const
{
  return map_received_ && map_.info.width > 0 && map_.info.height > 0;
}

bool LidarSimNode::lookupLaserPose(double & x, double & y, double & yaw) const
{
  try {
    const auto transform = tf_buffer_.lookupTransform(
      map_frame_id_, laser_frame_id_, tf2::TimePointZero,
      tf2::durationFromSec(0.1));
    x = transform.transform.translation.x;
    y = transform.transform.translation.y;
    yaw = tf2::getYaw(transform.transform.rotation);
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(yaw);
  } catch (const tf2::TransformException & ex) {
    RCLCPP_WARN_THROTTLE(
      get_logger(), *get_clock(), 2000,
      "Could not transform %s -> %s: %s",
      map_frame_id_.c_str(), laser_frame_id_.c_str(), ex.what());
    return false;
  }
}

bool LidarSimNode::isInsideMap(double map_x, double map_y) const
{
  const double resolution = map_.info.resolution;
  const int width = static_cast<int>(map_.info.width);
  const int height = static_cast<int>(map_.info.height);
  const double origin_x = map_.info.origin.position.x;
  const double origin_y = map_.info.origin.position.y;

  const int mx = static_cast<int>((map_x - origin_x) / resolution);
  const int my = static_cast<int>((map_y - origin_y) / resolution);
  return mx >= 0 && mx < width && my >= 0 && my < height;
}

LidarSimNode::CellState LidarSimNode::getCellState(double map_x, double map_y) const
{
  if (!isInsideMap(map_x, map_y)) {
    return CellState::Outside;
  }

  const double resolution = map_.info.resolution;
  const int width = static_cast<int>(map_.info.width);
  const double origin_x = map_.info.origin.position.x;
  const double origin_y = map_.info.origin.position.y;

  const int mx = static_cast<int>((map_x - origin_x) / resolution);
  const int my = static_cast<int>((map_y - origin_y) / resolution);
  const int8_t value = map_.data[static_cast<size_t>(my * width + mx)];

  if (value < 0) {
    return CellState::Unknown;
  }
  if (value >= occupied_threshold_) {
    return CellState::Occupied;
  }
  return CellState::Free;
}

double LidarSimNode::castRay(double origin_x, double origin_y, double ray_yaw) const
{
  const auto origin_state = getCellState(origin_x, origin_y);
  if (origin_state == CellState::Outside || origin_state == CellState::Unknown) {
    return std::numeric_limits<double>::infinity();
  }

  const double step = map_.info.resolution;
  const double cos_yaw = std::cos(ray_yaw);
  const double sin_yaw = std::sin(ray_yaw);

  double distance = step;
  while (distance <= range_max_) {
    const double x = origin_x + distance * cos_yaw;
    const double y = origin_y + distance * sin_yaw;
    switch (getCellState(x, y)) {
      case CellState::Outside:
        return range_max_;
      case CellState::Unknown:
        // 未定義領域では反射点を出さない
        return std::numeric_limits<double>::infinity();
      case CellState::Occupied:
        return std::max(distance, range_min_);
      case CellState::Free:
        break;
    }
    distance += step;
  }
  return range_max_;
}

double LidarSimNode::sampleGaussian(double stddev)
{
  if (stddev <= 0.0) {
    return 0.0;
  }
  return normal_dist_(rng_) * stddev;
}

double LidarSimNode::applyRangeNoise(double true_range)
{
  if (!enable_noise_) {
    return true_range;
  }

  // 欠測（実機 LiDAR の drop frame / 反射失敗）
  if (uniform_dist_(rng_) < dropout_probability_) {
    return std::numeric_limits<double>::infinity();
  }

  const bool is_hit = true_range + 1e-3 < range_max_;
  if (!is_hit) {
    return true_range;
  }

  double noisy = true_range + sampleGaussian(range_stddev_);

  if (range_resolution_ > 1e-6) {
    noisy = std::round(noisy / range_resolution_) * range_resolution_;
  }

  return std::clamp(noisy, range_min_, range_max_);
}

float LidarSimNode::applyIntensityNoise(float base_intensity)
{
  if (!enable_noise_ || base_intensity <= 0.0f) {
    return base_intensity;
  }
  const float noisy = base_intensity + static_cast<float>(sampleGaussian(intensity_stddev_));
  return std::max(0.0f, noisy);
}

void LidarSimNode::publishScan()
{
  if (!mapReceived()) {
    return;
  }

  double laser_x = 0.0;
  double laser_y = 0.0;
  double laser_yaw = 0.0;
  if (!lookupLaserPose(laser_x, laser_y, laser_yaw)) {
    return;
  }

  if (!isInsideMap(laser_x, laser_y)) {
    RCLCPP_WARN_THROTTLE(
      get_logger(), *get_clock(), 3000,
      "Laser pose (%.2f, %.2f) is outside map. Set 2D Pose Estimate on free space.",
      laser_x, laser_y);
  } else if (getCellState(laser_x, laser_y) == CellState::Unknown) {
    RCLCPP_WARN_THROTTLE(
      get_logger(), *get_clock(), 3000,
      "Laser pose (%.2f, %.2f) is in unknown map cells.",
      laser_x, laser_y);
  }

  sensor_msgs::msg::LaserScan scan;
  scan.header.stamp = now();
  scan.header.frame_id = laser_frame_id_;
  scan.angle_min = angle_min_;
  scan.angle_max = angle_max_;
  scan.angle_increment = (angle_max_ - angle_min_) / static_cast<double>(num_beams_ - 1);
  scan.time_increment = 0.0;
  scan.scan_time = 1.0 / publish_rate_;
  scan.range_min = range_min_;
  scan.range_max = range_max_;
  scan.ranges.resize(static_cast<size_t>(num_beams_), range_max_);
  scan.intensities.resize(static_cast<size_t>(num_beams_), 0.0f);

  for (int i = 0; i < num_beams_; ++i) {
    const double beam_angle = angle_min_ + static_cast<double>(i) * scan.angle_increment;
    const double ray_yaw = laser_yaw + beam_angle;
    const double true_range = castRay(laser_x, laser_y, ray_yaw);
    const double range = applyRangeNoise(true_range);
    const bool valid = std::isfinite(range) && range >= range_min_ && range <= range_max_;
    scan.ranges[static_cast<size_t>(i)] = valid ? range : std::numeric_limits<float>::infinity();
    scan.intensities[static_cast<size_t>(i)] = valid ?
      applyIntensityNoise(100.0f) : 0.0f;
  }

  scan_publisher_->publish(scan);
}

}  // namespace robot_sim

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(robot_sim::LidarSimNode)
