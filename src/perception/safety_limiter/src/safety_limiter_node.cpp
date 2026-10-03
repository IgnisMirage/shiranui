#include "safety_limiter/safety_limiter_node.hpp"

#include <tf2/LinearMath/Quaternion.h>
#include <tf2/utils.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <tf2_sensor_msgs/tf2_sensor_msgs.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace safety_limiter
{
SafetyLimiterNode::SafetyLimiterNode(const rclcpp::NodeOptions & options)
: Node("safety_limiter", options),
  has_cmd_vel_(false),
  has_cloud_(false),
  has_footprint_(false)
{
  robot_frame_ = declare_parameter<std::string>("robot_frame", "base_link");
  map_frame_ = declare_parameter<std::string>("map_frame", "map");
  publish_rate_ = declare_parameter<double>("publish_rate", 10.0);
  prediction_time_ = declare_parameter<double>("prediction_time", 2.0);
  prediction_step_ = declare_parameter<double>("prediction_step", 0.1);
  // これより古い指令は停止 (0) とみなす。joy は押している間しか指令が来ず、
  // プランナーもゴール到達後は止まるため、最後の指令を出し続けないようにする
  cmd_vel_timeout_ = declare_parameter<double>("cmd_vel_timeout", 0.5);
  footprint_margin_ = declare_parameter<double>("footprint_margin", 0.15);
  // これより古い点群は無効とみなす (LiDAR 停止時に古い点群で「衝突なし」と判定し続けない)
  cloud_timeout_ = declare_parameter<double>("cloud_timeout", 0.5);
  // true: 点群・footprint・自己位置のいずれかが得られない/古いとき停止する (fail-safe)
  fail_safe_stop_ = declare_parameter<bool>("fail_safe_stop", true);
  // 許容減速度。障害物までの距離 d から許容速度 sqrt(2*a*d) を求めて減速する
  max_decel_ = std::max(declare_parameter<double>("max_decel", 0.5), 1e-3);
  // 実 footprint が障害物に触れる手前で止まるための余裕距離 [m]
  stop_distance_ = declare_parameter<double>("stop_distance", 0.1);
  // margin 内に障害物がある間の最低速度 [m/s] (margin 内からの脱出用)
  margin_min_speed_ = declare_parameter<double>("margin_min_speed", 0.1);
  // 減速後の速度倍率の回復レート [1/s] (減速は即時、復帰は徐々に)
  recovery_rate_ = declare_parameter<double>("recovery_rate", 1.0);
  enable_visualization_ = declare_parameter<bool>("enable_visualization", true);
  visualization_stride_ = declare_parameter<int>("visualization_stride", 3);

  cmd_vel_in_topic_ = declare_parameter<std::string>("cmd_vel_in_topic", "cmd_vel_in");
  cmd_vel_out_topic_ = declare_parameter<std::string>("cmd_vel_out_topic", "cmd_vel");
  cloud_topic_ = declare_parameter<std::string>("cloud_topic", "cloud");
  footprint_topic_ = declare_parameter<std::string>("footprint_topic", "footprint");
  future_motion_prediction_topic_ = declare_parameter<std::string>(
    "future_motion_prediction_topic", "future_motion_prediction");
  future_motion_markers_topic_ = declare_parameter<std::string>(
    "future_motion_markers_topic", "future_motion_markers");
  cloud_in_map_topic_ = declare_parameter<std::string>("cloud_in_map_topic", "cloud_in_map");
  collision_topic_ = declare_parameter<std::string>("collision_topic", "collision");
  collision_margin_topic_ = declare_parameter<std::string>(
    "collision_margin_topic", "collision_margin");

  latest_cmd_vel_ = std::make_shared<geometry_msgs::msg::Twist>();

  // cmd_vel: pure_pursuit 等 (Reliable, depth=10)
  const rclcpp::QoS cmd_vel_qos(10);
  // cloud: laserscan_to_pointcloud / LiDAR driver (Best Effort, SensorDataQoS)
  const rclcpp::QoS cloud_qos = rclcpp::SensorDataQoS();
  // footprint: footprint_publisher (Transient Local, Reliable)
  const rclcpp::QoS footprint_qos = rclcpp::QoS(1).transient_local().reliable();

  collision_pub_ = create_publisher<std_msgs::msg::Bool>(collision_topic_, rclcpp::QoS(10));
  collision_margin_pub_ = create_publisher<std_msgs::msg::Bool>(
    collision_margin_topic_, rclcpp::QoS(10));
  cmd_vel_pub_ = create_publisher<geometry_msgs::msg::Twist>(cmd_vel_out_topic_, cmd_vel_qos);
  future_motion_prediction_pub_ = create_publisher<nav_msgs::msg::Path>(
    future_motion_prediction_topic_, rclcpp::QoS(10));
  future_motion_markers_pub_ = create_publisher<visualization_msgs::msg::MarkerArray>(
    future_motion_markers_topic_, rclcpp::QoS(10));
  cloud_in_map_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>(
    cloud_in_map_topic_, cloud_qos);

  cmd_vel_sub_ = create_subscription<geometry_msgs::msg::Twist>(
    cmd_vel_in_topic_, cmd_vel_qos,
    std::bind(&SafetyLimiterNode::cmdVelCallback, this, std::placeholders::_1));
  point_cloud_sub_ = create_subscription<sensor_msgs::msg::PointCloud2>(
    cloud_topic_, cloud_qos,
    std::bind(&SafetyLimiterNode::pointCloudCallback, this, std::placeholders::_1));
  footprint_sub_ = create_subscription<geometry_msgs::msg::PolygonStamped>(
    footprint_topic_, footprint_qos,
    std::bind(&SafetyLimiterNode::footprintCallback, this, std::placeholders::_1));

  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  const auto period = std::chrono::duration<double>(1.0 / publish_rate_);
  timer_ = create_wall_timer(
    std::chrono::duration_cast<std::chrono::nanoseconds>(period),
    std::bind(&SafetyLimiterNode::timerCallback, this));

  RCLCPP_INFO(get_logger(),
    "Safety limiter started (map=%s, robot=%s)", map_frame_.c_str(), robot_frame_.c_str());
  RCLCPP_INFO(get_logger(),
    "Sub: cmd_vel_in=%s (Reliable), cloud=%s (BestEffort), footprint=%s (TransientLocal)",
    cmd_vel_in_topic_.c_str(), cloud_topic_.c_str(), footprint_topic_.c_str());
  RCLCPP_INFO(get_logger(), "Pub: cmd_vel_out=%s (stop on /%s)",
    cmd_vel_out_topic_.c_str(), collision_topic_.c_str());
  RCLCPP_INFO(get_logger(),
    "Pub: future_motion_prediction=%s, future_motion_markers=%s, cloud_in_map=%s (BestEffort)",
    future_motion_prediction_topic_.c_str(),
    future_motion_markers_topic_.c_str(),
    cloud_in_map_topic_.c_str());
}

void SafetyLimiterNode::cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg)
{
  latest_cmd_vel_ = msg;
  last_cmd_vel_time_ = now();
  has_cmd_vel_ = true;
}

void SafetyLimiterNode::pointCloudCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg)
{
  geometry_msgs::msg::TransformStamped transform;
  const rclcpp::Time stamp(msg->header.stamp);
  bool got_transform = false;
  if (stamp.nanoseconds() != 0) {
    // 点群の取得時刻の TF を使う (走行中の変換ずれを減らす)
    try {
      transform = tf_buffer_->lookupTransform(
        map_frame_, msg->header.frame_id, stamp);
      got_transform = true;
    } catch (const tf2::TransformException &) {
    }
  }
  if (!got_transform) {
    try {
      transform = tf_buffer_->lookupTransform(
        map_frame_, msg->header.frame_id, tf2::TimePointZero);
    } catch (const tf2::TransformException & ex) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
        "Could not transform cloud to %s: %s", map_frame_.c_str(), ex.what());
      return;
    }
  }

  sensor_msgs::msg::PointCloud2 transformed_cloud;
  tf2::doTransform(*msg, transformed_cloud, transform);
  latest_cloud_ = std::make_shared<sensor_msgs::msg::PointCloud2>(transformed_cloud);

  cloud_points_map_.clear();
  cloud_points_map_.reserve(transformed_cloud.width * transformed_cloud.height);

  sensor_msgs::PointCloud2ConstIterator<float> iter_x(transformed_cloud, "x");
  sensor_msgs::PointCloud2ConstIterator<float> iter_y(transformed_cloud, "y");
  for (; iter_x != iter_x.end(); ++iter_x, ++iter_y) {
    if (!std::isfinite(*iter_x) || !std::isfinite(*iter_y)) {
      continue;
    }
    cloud_points_map_.push_back({*iter_x, *iter_y});
  }

  has_cloud_ = true;
  last_cloud_time_ = now();
  cloud_in_map_pub_->publish(transformed_cloud);

  RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 3000,
    "Cloud received: frame=%s -> %s, valid_points=%zu",
    msg->header.frame_id.c_str(), map_frame_.c_str(), cloud_points_map_.size());
}

void SafetyLimiterNode::footprintCallback(
  const geometry_msgs::msg::PolygonStamped::SharedPtr msg)
{
  footprint_local_.clear();
  for (const auto & point : msg->polygon.points) {
    footprint_local_.push_back({point.x, point.y});
  }
  footprint_radius_ = 0.0;
  for (const auto & p : footprint_local_) {
    footprint_radius_ = std::max(footprint_radius_, std::hypot(p.x, p.y));
  }
  has_footprint_ = !footprint_local_.empty();
}

bool SafetyLimiterNode::getCurrentPose(geometry_msgs::msg::Pose & pose) const
{
  try {
    const auto transform = tf_buffer_->lookupTransform(
      map_frame_, robot_frame_, tf2::TimePointZero);
    pose.position.x = transform.transform.translation.x;
    pose.position.y = transform.transform.translation.y;
    pose.position.z = transform.transform.translation.z;
    pose.orientation = transform.transform.rotation;
    return true;
  } catch (const tf2::TransformException & ex) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
      "Could not get robot pose (%s -> %s): %s",
      map_frame_.c_str(), robot_frame_.c_str(), ex.what());
    return false;
  }
}

void SafetyLimiterNode::predictTrajectory(
  const geometry_msgs::msg::Pose & start_pose,
  std::vector<geometry_msgs::msg::Pose> & predicted_poses) const
{
  predicted_poses.clear();
  if (!has_cmd_vel_) {
    return;
  }

  const double yaw = tf2::getYaw(start_pose.orientation);

  const double vx = latest_cmd_vel_->linear.x;
  const double vy = latest_cmd_vel_->linear.y;
  const double omega = latest_cmd_vel_->angular.z;

  geometry_msgs::msg::Pose predicted_pose = start_pose;
  double predicted_yaw = yaw;

  predicted_poses.push_back(start_pose);
  const int steps = static_cast<int>(std::round(prediction_time_ / prediction_step_));
  for (int i = 0; i < steps; ++i) {
    predicted_pose.position.x +=
      (vx * std::cos(predicted_yaw) - vy * std::sin(predicted_yaw)) * prediction_step_;
    predicted_pose.position.y +=
      (vx * std::sin(predicted_yaw) + vy * std::cos(predicted_yaw)) * prediction_step_;

    predicted_yaw += omega * prediction_step_;

    tf2::Quaternion q;
    q.setRPY(0.0, 0.0, predicted_yaw);
    predicted_pose.orientation = tf2::toMsg(q);
    predicted_poses.push_back(predicted_pose);
  }
}

std::vector<SafetyLimiterNode::Point2D> SafetyLimiterNode::transformFootprint(
  const geometry_msgs::msg::Pose & pose,
  const std::vector<Point2D> & local_footprint) const
{
  const double yaw = tf2::getYaw(pose.orientation);
  const double cos_yaw = std::cos(yaw);
  const double sin_yaw = std::sin(yaw);

  std::vector<Point2D> world_footprint;
  world_footprint.reserve(local_footprint.size());
  for (const auto & local : local_footprint) {
    Point2D world;
    world.x = pose.position.x + cos_yaw * local.x - sin_yaw * local.y;
    world.y = pose.position.y + sin_yaw * local.x + cos_yaw * local.y;
    world_footprint.push_back(world);
  }
  return world_footprint;
}

std::vector<SafetyLimiterNode::Point2D> SafetyLimiterNode::expandFootprint(
  const std::vector<Point2D> & local_footprint, double margin) const
{
  if (local_footprint.empty() || margin <= 0.0) {
    return local_footprint;
  }

  const auto center = vertexCentroid(local_footprint);
  const double cx = center.x;
  const double cy = center.y;

  std::vector<Point2D> expanded;
  expanded.reserve(local_footprint.size());
  for (const auto & point : local_footprint) {
    const double dx = point.x - cx;
    const double dy = point.y - cy;
    const double dist = std::hypot(dx, dy);
    if (dist < 1e-6) {
      expanded.push_back(point);
      continue;
    }
    const double scale = (dist + margin) / dist;
    expanded.push_back({cx + dx * scale, cy + dy * scale});
  }
  return expanded;
}

bool SafetyLimiterNode::pointInPolygon(
  double x, double y, const std::vector<Point2D> & polygon)
{
  if (polygon.size() < 3) {
    return false;
  }

  bool inside = false;
  for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
    const auto & pi = polygon[i];
    const auto & pj = polygon[j];
    const bool intersect = ((pi.y > y) != (pj.y > y)) &&
      (x < (pj.x - pi.x) * (y - pi.y) / (pj.y - pi.y) + pi.x);
    if (intersect) {
      inside = !inside;
    }
  }
  return inside;
}

SafetyLimiterNode::Point2D SafetyLimiterNode::vertexCentroid(
  const std::vector<Point2D> & polygon)
{
  Point2D sum{0.0, 0.0};
  for (const auto & p : polygon) {
    sum.x += p.x;
    sum.y += p.y;
  }
  const double n = static_cast<double>(polygon.size());
  return {sum.x / n, sum.y / n};
}

int SafetyLimiterNode::firstCollisionIndex(
  const std::vector<geometry_msgs::msg::Pose> & predicted_poses,
  const std::vector<Point2D> & local_footprint) const
{
  if (predicted_poses.empty() || local_footprint.empty() || cloud_points_map_.empty()) {
    return -1;
  }

  for (size_t i = 0; i < predicted_poses.size(); ++i) {
    const auto world_footprint = transformFootprint(predicted_poses[i], local_footprint);
    for (const auto & cloud_point : cloud_points_map_) {
      if (pointInPolygon(cloud_point.x, cloud_point.y, world_footprint)) {
        return static_cast<int>(i);
      }
    }
  }
  return -1;
}

void SafetyLimiterNode::publishFutureMotionVisualization(
  const std::vector<geometry_msgs::msg::Pose> & predicted_poses,
  bool collision, bool collision_margin) const
{
  if (!enable_visualization_) {
    return;
  }

  nav_msgs::msg::Path path;
  path.header.stamp = now();
  path.header.frame_id = map_frame_;
  for (const auto & pose : predicted_poses) {
    geometry_msgs::msg::PoseStamped pose_stamped;
    pose_stamped.header = path.header;
    pose_stamped.pose = pose;
    path.poses.push_back(pose_stamped);
  }
  future_motion_prediction_pub_->publish(path);

  visualization_msgs::msg::MarkerArray markers;
  int marker_id = 0;
  const int stride = std::max(1, visualization_stride_);
  const auto expanded_footprint = expandFootprint(footprint_local_, footprint_margin_);

  for (size_t i = 0; i < predicted_poses.size(); i += static_cast<size_t>(stride)) {
    const auto & pose = predicted_poses[i];
    const auto world_footprint = transformFootprint(pose, footprint_local_);
    const auto world_margin = transformFootprint(pose, expanded_footprint);

    visualization_msgs::msg::Marker footprint_marker;
    footprint_marker.header.stamp = now();
    footprint_marker.header.frame_id = map_frame_;
    footprint_marker.ns = "footprint";
    footprint_marker.id = marker_id++;
    footprint_marker.type = visualization_msgs::msg::Marker::LINE_STRIP;
    footprint_marker.action = visualization_msgs::msg::Marker::ADD;
    footprint_marker.pose.orientation.w = 1.0;
    footprint_marker.scale.x = 0.02;
    footprint_marker.color.r = collision ? 1.0f : 0.0f;
    footprint_marker.color.g = collision ? 0.0f : 1.0f;
    footprint_marker.color.b = 0.0f;
    footprint_marker.color.a = 0.8f;
    footprint_marker.lifetime = rclcpp::Duration::from_seconds(0.3);
    for (const auto & point : world_footprint) {
      geometry_msgs::msg::Point p;
      p.x = point.x;
      p.y = point.y;
      p.z = pose.position.z;
      footprint_marker.points.push_back(p);
    }
    if (!world_footprint.empty()) {
      geometry_msgs::msg::Point p;
      p.x = world_footprint.front().x;
      p.y = world_footprint.front().y;
      p.z = pose.position.z;
      footprint_marker.points.push_back(p);
    }
    markers.markers.push_back(footprint_marker);

    visualization_msgs::msg::Marker margin_marker = footprint_marker;
    margin_marker.ns = "footprint_margin";
    margin_marker.id = marker_id++;
    margin_marker.scale.x = 0.015;
    margin_marker.color.r = 1.0f;
    margin_marker.color.g = collision_margin ? 0.5f : 1.0f;
    margin_marker.color.b = 0.0f;
    margin_marker.color.a = 0.45f;
    margin_marker.points.clear();
    for (const auto & point : world_margin) {
      geometry_msgs::msg::Point p;
      p.x = point.x;
      p.y = point.y;
      p.z = pose.position.z;
      margin_marker.points.push_back(p);
    }
    if (!world_margin.empty()) {
      geometry_msgs::msg::Point p;
      p.x = world_margin.front().x;
      p.y = world_margin.front().y;
      p.z = pose.position.z;
      margin_marker.points.push_back(p);
    }
    markers.markers.push_back(margin_marker);
  }

  future_motion_markers_pub_->publish(markers);
}

void SafetyLimiterNode::publishCmdVel(double scale)
{
  geometry_msgs::msg::Twist cmd_vel;
  if (has_cmd_vel_ && scale > 0.0) {
    cmd_vel = *latest_cmd_vel_;
    cmd_vel.linear.x *= scale;
    cmd_vel.linear.y *= scale;
    cmd_vel.angular.z *= scale;
  }
  cmd_vel_pub_->publish(cmd_vel);
}

void SafetyLimiterNode::publishStop(bool fail_safe)
{
  std_msgs::msg::Bool collision_msg;
  std_msgs::msg::Bool collision_margin_msg;
  collision_msg.data = false;
  collision_margin_msg.data = false;
  collision_pub_->publish(collision_msg);
  collision_margin_pub_->publish(collision_margin_msg);
  // fail_safe=true なら入力欠損として停止、false なら従来どおり素通し
  speed_scale_ = (fail_safe && fail_safe_stop_) ? 0.0 : 1.0;
  publishCmdVel(speed_scale_);
}

void SafetyLimiterNode::timerCallback()
{
  std_msgs::msg::Bool collision_msg;
  std_msgs::msg::Bool collision_margin_msg;
  collision_msg.data = false;
  collision_margin_msg.data = false;

  if (has_cmd_vel_ && (now() - last_cmd_vel_time_).seconds() > cmd_vel_timeout_) {
    latest_cmd_vel_ = std::make_shared<geometry_msgs::msg::Twist>();
  }

  if (!has_footprint_ || !has_cloud_ || !has_cmd_vel_) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 3000,
      "Waiting for inputs (footprint=%d, cloud=%d, cmd_vel=%d): stopping=%d",
      has_footprint_, has_cloud_, has_cmd_vel_, fail_safe_stop_);
    publishStop(true);
    return;
  }

  if ((now() - last_cloud_time_).seconds() > cloud_timeout_) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000,
      "Cloud is stale (> %.2fs): stopping=%d", cloud_timeout_, fail_safe_stop_);
    publishStop(true);
    return;
  }

  geometry_msgs::msg::Pose current_pose;
  if (!getCurrentPose(current_pose)) {
    publishStop(true);
    return;
  }

  std::vector<geometry_msgs::msg::Pose> predicted_poses;
  predictTrajectory(current_pose, predicted_poses);

  const int collision_index = firstCollisionIndex(predicted_poses, footprint_local_);
  const auto expanded_footprint = expandFootprint(footprint_local_, footprint_margin_);
  const int margin_index = firstCollisionIndex(predicted_poses, expanded_footprint);
  const bool collision = collision_index >= 0;
  const bool collision_margin = margin_index >= 0;

  // 並進速度と、外周が回転で動く速度 (omega * 外接半径) を合わせた代表速度 [m/s]
  const double speed = std::hypot(latest_cmd_vel_->linear.x, latest_cmd_vel_->linear.y) +
    std::fabs(latest_cmd_vel_->angular.z) * footprint_radius_;

  double target_scale = 1.0;
  if (speed > 1e-3) {
    if (collision_index == 0) {
      target_scale = 0.0;
    } else {
      if (collision_index > 0) {
        const double dist = speed * collision_index * prediction_step_ - stop_distance_;
        target_scale = std::min(
          target_scale, std::sqrt(2.0 * max_decel_ * std::max(dist, 0.0)) / speed);
      }
      if (margin_index >= 0) {
        const double dist = speed * margin_index * prediction_step_;
        const double allowed = std::max(std::sqrt(2.0 * max_decel_ * dist), margin_min_speed_);
        target_scale = std::min(target_scale, allowed / speed);
      }
    }
    if (speed / max_decel_ > prediction_time_) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000,
        "prediction_time (%.2fs) is shorter than the stopping time (%.2fs)",
        prediction_time_, speed / max_decel_);
    }
  }
  target_scale = std::clamp(target_scale, 0.0, 1.0);
  if (target_scale < speed_scale_) {
    speed_scale_ = target_scale;
  } else {
    speed_scale_ = std::min(target_scale, speed_scale_ + recovery_rate_ / publish_rate_);
  }

  collision_msg.data = collision;
  collision_margin_msg.data = collision_margin;
  collision_pub_->publish(collision_msg);
  collision_margin_pub_->publish(collision_margin_msg);

  publishFutureMotionVisualization(predicted_poses, collision, collision_margin);
  publishCmdVel(speed_scale_);

  if (collision) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000,
      "Collision predicted in %d steps (footprint): speed scale %.2f",
      collision_index, speed_scale_);
  } else if (collision_margin) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000,
      "Obstacle in margin (%d steps): speed scale %.2f", margin_index, speed_scale_);
  }
}

}  // namespace safety_limiter

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<safety_limiter::SafetyLimiterNode>());
  rclcpp::shutdown();
  return 0;
}
