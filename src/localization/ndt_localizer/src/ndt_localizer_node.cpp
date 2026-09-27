#include <pcl/common/transforms.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pclomp/ndt_omp.h>

#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <std_msgs/msg/float32.hpp>
#include <std_msgs/msg/int32.hpp>
#include <tf2_eigen/tf2_eigen.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ndt_localizer
{

using PointCloud = pcl::PointCloud<pcl::PointXYZ>;

constexpr size_t kMinScanPoints = 100;
constexpr double kImuBufferSec = 1.0;
constexpr double kMinImuRateHz = 50.0;
// After this many consecutive rejections the odometry prediction itself is suspect,
// so only the NVTL check is applied.
constexpr int kJumpCheckMaxRejects = 10;
constexpr double kMinPositionVariance = 0.01;
constexpr double kMinRotationVariance = 0.0003;

pclomp::NeighborSearchMethod searchMethodFromInt(int value)
{
  switch (value) {
    case 0:
      return pclomp::KDTREE;
    case 2:
      return pclomp::DIRECT1;
    case 1:
    default:
      return pclomp::DIRECT7;
  }
}

PointCloud::Ptr voxelDownsample(const PointCloud::Ptr & cloud, float leaf_size)
{
  auto filtered = std::make_shared<PointCloud>();
  if (leaf_size <= 0.0F) {
    *filtered = *cloud;
    return filtered;
  }
  pcl::VoxelGrid<pcl::PointXYZ> vg;
  vg.setLeafSize(leaf_size, leaf_size, leaf_size);
  vg.setInputCloud(cloud);
  vg.filter(*filtered);
  return filtered;
}

Eigen::Affine3f toAffine3f(const Eigen::Isometry3d & T)
{
  return Eigen::Affine3f(T.matrix().cast<float>());
}

geometry_msgs::msg::Pose isometryToPoseMsg(const Eigen::Isometry3d & T)
{
  geometry_msgs::msg::Pose pose;
  pose.position.x = T.translation().x();
  pose.position.y = T.translation().y();
  pose.position.z = T.translation().z();
  const Eigen::Quaterniond q(T.rotation());
  pose.orientation.x = q.x();
  pose.orientation.y = q.y();
  pose.orientation.z = q.z();
  pose.orientation.w = q.w();
  return pose;
}

double yawOf(const Eigen::Isometry3d & T)
{
  const Eigen::Matrix3d R = T.linear();
  return std::atan2(R(1, 0), R(0, 0));
}

bool hasField(const sensor_msgs::msg::PointCloud2 & msg, const std::string & name, uint8_t datatype)
{
  return std::any_of(msg.fields.begin(), msg.fields.end(), [&](const auto & field) {
    return field.name == name && field.datatype == datatype;
  });
}

std::array<double, 36> covarianceFromHessian(const Eigen::Matrix<double, 6, 6> & hessian)
{
  // Laplace approximation: the NDT score is maximized, so the covariance is -H^-1.
  Eigen::Matrix<double, 6, 6> cov = -hessian.inverse();
  bool valid = cov.allFinite();
  for (int i = 0; i < 6 && valid; ++i) {
    valid = cov(i, i) > 0.0;
  }
  if (!valid) {
    cov.setZero();
    cov.diagonal() << 1.0, 1.0, 1.0, 0.1, 0.1, 0.1;
  }
  for (int i = 0; i < 3; ++i) {
    cov(i, i) = std::max(cov(i, i), kMinPositionVariance);
    cov(i + 3, i + 3) = std::max(cov(i + 3, i + 3), kMinRotationVariance);
  }

  std::array<double, 36> out{};
  for (int r = 0; r < 6; ++r) {
    for (int c = 0; c < 6; ++c) {
      out[r * 6 + c] = cov(r, c);
    }
  }
  return out;
}

class NdtLocalizerNode : public rclcpp::Node
{
public:
  NdtLocalizerNode()
  : Node("ndt_localizer")
  {
    map_pcd_path_ = declare_parameter<std::string>("map_pcd_path", "");
    map_frame_ = declare_parameter<std::string>("map_frame", "map");
    odom_frame_ = declare_parameter<std::string>("odom_frame", "odom");
    base_frame_ = declare_parameter<std::string>("base_frame", "base_link");
    const std::string input_topic = declare_parameter<std::string>("input_topic", "/mid360/points");
    const std::string imu_topic = declare_parameter<std::string>("imu_topic", "/mid360/imu");

    ndt_resolution_ = declare_parameter<double>("ndt_resolution", 2.0);
    ndt_step_size_ = declare_parameter<double>("ndt_step_size", 0.1);
    ndt_transformation_epsilon_ = declare_parameter<double>("ndt_transformation_epsilon", 0.01);
    ndt_max_iterations_ = declare_parameter<int>("ndt_max_iterations", 30);
    ndt_num_threads_ = declare_parameter<int>("ndt_num_threads", 4);
    ndt_search_method_ = declare_parameter<int>("ndt_search_method", 1);

    source_voxel_leaf_size_ =
      static_cast<float>(declare_parameter<double>("source_voxel_leaf_size", 0.5));
    map_voxel_leaf_size_ = static_cast<float>(declare_parameter<double>("map_voxel_leaf_size", 0.5));
    min_scan_range_ = declare_parameter<double>("min_scan_range", 1.0);
    max_scan_range_ = declare_parameter<double>("max_scan_range", 40.0);
    min_height_ = declare_parameter<double>("min_height", -2.0);
    max_height_ = declare_parameter<double>("max_height", 4.0);

    deskew_ = declare_parameter<bool>("deskew", true);
    deskew_use_imu_ = declare_parameter<bool>("deskew_use_imu", true);
    scan_accumulation_count_ =
      std::max<int>(1, static_cast<int>(declare_parameter<int>("scan_accumulation_count", 3)));

    nvtl_threshold_ = declare_parameter<double>("nvtl_threshold", 2.3);
    max_translation_jump_ = declare_parameter<double>("max_translation_jump", 1.0);
    max_yaw_jump_ = declare_parameter<double>("max_yaw_jump_deg", 10.0) * M_PI / 180.0;

    publish_tf_ = declare_parameter<bool>("publish_tf", true);
    const std::string pose_topic = declare_parameter<std::string>("pose_topic", "ndt_pose");
    const std::string odom_topic = declare_parameter<std::string>("odom_topic", "localization_odom");

    if (map_pcd_path_.empty()) {
      RCLCPP_ERROR(get_logger(), "map_pcd_path is empty. Set a PCD path for the NDT target map.");
      return;
    }
    if (!loadMap(map_pcd_path_)) {
      return;
    }
    configureNdt();

    tf_buffer_ = std::make_shared<tf2_ros::Buffer>(get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);
    tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

    pose_pub_ = create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>(pose_topic, 10);
    odom_pub_ = create_publisher<nav_msgs::msg::Odometry>(odom_topic, 10);
    nvtl_pub_ = create_publisher<std_msgs::msg::Float32>("~/nvtl", 10);
    iteration_pub_ = create_publisher<std_msgs::msg::Int32>("~/iteration_num", 10);
    aligned_pub_ =
      create_publisher<sensor_msgs::msg::PointCloud2>("~/aligned_points", rclcpp::SensorDataQoS());

    if (deskew_ && deskew_use_imu_) {
      imu_group_ = create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
      rclcpp::SubscriptionOptions imu_options;
      imu_options.callback_group = imu_group_;
      imu_sub_ = create_subscription<sensor_msgs::msg::Imu>(
        imu_topic, rclcpp::SensorDataQoS().keep_last(400),
        [this](const sensor_msgs::msg::Imu::SharedPtr msg) { onImu(msg); }, imu_options);
    }

    initial_pose_sub_ = create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
      "initialpose", rclcpp::QoS(10),
      [this](const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg) {
        onInitialPose(msg);
      });

    cloud_sub_ = create_subscription<sensor_msgs::msg::PointCloud2>(
      input_topic, rclcpp::SensorDataQoS(),
      [this](const sensor_msgs::msg::PointCloud2::SharedPtr msg) { onCloud(msg); });

    RCLCPP_INFO(
      get_logger(),
      "NDT localizer ready. map=%s, cloud=%s, deskew=%s (imu=%s), accumulation=%d. "
      "Publish /initialpose to start.",
      map_pcd_path_.c_str(), input_topic.c_str(), deskew_ ? "on" : "off",
      deskew_use_imu_ ? "on" : "off", scan_accumulation_count_);
  }

private:
  struct GyroSample
  {
    rclcpp::Time stamp;
    Eigen::Vector3d angular_velocity;
  };

  bool loadMap(const std::string & path)
  {
    PointCloud::Ptr raw(new PointCloud());
    if (pcl::io::loadPCDFile(path, *raw) < 0) {
      RCLCPP_ERROR(get_logger(), "Failed to load map PCD: %s", path.c_str());
      return false;
    }
    map_cloud_ = voxelDownsample(raw, map_voxel_leaf_size_);
    RCLCPP_INFO(
      get_logger(), "Loaded map %zu -> %zu points (voxel %.2f m)", raw->size(), map_cloud_->size(),
      map_voxel_leaf_size_);
    return !map_cloud_->empty();
  }

  void configureNdt()
  {
    ndt_ = std::make_shared<pclomp::NormalDistributionsTransform<pcl::PointXYZ, pcl::PointXYZ>>();
    ndt_->setResolution(static_cast<float>(ndt_resolution_));
    ndt_->setStepSize(ndt_step_size_);
    ndt_->setTransformationEpsilon(ndt_transformation_epsilon_);
    ndt_->setMaximumIterations(ndt_max_iterations_);
    ndt_->setNumThreads(ndt_num_threads_);
    ndt_->setNeighborhoodSearchMethod(searchMethodFromInt(ndt_search_method_));
    ndt_->setInputTarget(map_cloud_);
  }

  void onImu(const sensor_msgs::msg::Imu::SharedPtr msg)
  {
    const rclcpp::Time stamp(msg->header.stamp);
    std::lock_guard<std::mutex> lock(imu_mutex_);
    imu_frame_ = msg->header.frame_id;
    imu_buffer_.push_back(
      {stamp, Eigen::Vector3d(
                msg->angular_velocity.x, msg->angular_velocity.y, msg->angular_velocity.z)});
    while (!imu_buffer_.empty() && (stamp - imu_buffer_.front().stamp).seconds() > kImuBufferSec) {
      imu_buffer_.pop_front();
    }
  }

  void onInitialPose(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg)
  {
    if (msg->header.frame_id != map_frame_) {
      RCLCPP_WARN(
        get_logger(), "initialpose frame_id '%s' != map_frame '%s'", msg->header.frame_id.c_str(),
        map_frame_.c_str());
    }
    tf2::fromMsg(msg->pose.pose, initial_map_to_base_);
    pending_initial_pose_ = true;
    consecutive_rejects_ = 0;
    RCLCPP_INFO(
      get_logger(), "Initial pose set (x=%.2f, y=%.2f, yaw=%.1f deg)",
      initial_map_to_base_.translation().x(), initial_map_to_base_.translation().y(),
      yawOf(initial_map_to_base_) * 180.0 / M_PI);
  }

  void onCloud(const sensor_msgs::msg::PointCloud2::SharedPtr msg)
  {
    if (!ndt_) {
      return;
    }
    if (!has_pose_ && !pending_initial_pose_) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "Waiting for /initialpose...");
      return;
    }

    const rclcpp::Time stamp(msg->header.stamp);
    Eigen::Isometry3d base_to_sensor;
    if (!lookupStaticFromBase(msg->header.frame_id, base_to_sensor)) {
      return;
    }
    Eigen::Isometry3d odom_to_base;
    if (!lookupOdomToBase(stamp, 0.1, true, odom_to_base)) {
      return;
    }

    const PointCloud::Ptr frame = preprocess(*msg, stamp, base_to_sensor, odom_to_base);
    accumulate(frame, odom_to_base);
    const PointCloud::Ptr scan = mergedScan(odom_to_base);
    if (scan->size() < kMinScanPoints) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 3000, "Too few scan points after filter: %zu", scan->size());
      publishMapToOdom(stamp);
      return;
    }

    if (pending_initial_pose_) {
      map_to_odom_ = initial_map_to_base_ * odom_to_base.inverse();
      pending_initial_pose_ = false;
      has_pose_ = true;
      skip_jump_check_ = true;
    }

    const Eigen::Isometry3d guess = map_to_odom_ * odom_to_base;
    ndt_->setInputSource(scan);
    auto aligned = std::make_shared<PointCloud>();
    ndt_->align(*aligned, guess.matrix().cast<float>());
    const pclomp::NdtResult result = ndt_->getResult();
    publishDiagnostics(result);

    const Eigen::Isometry3d estimate(result.pose.cast<double>());
    const std::string reject_reason = validate(result, guess, estimate);
    if (reject_reason.empty()) {
      map_to_odom_ = estimate * odom_to_base.inverse();
      skip_jump_check_ = false;
      consecutive_rejects_ = 0;
      publishEstimate(stamp, estimate, result.hessian);
      publishAligned(stamp, aligned);
    } else {
      ++consecutive_rejects_;
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 1000,
        "NDT result rejected (%s): NVTL=%.2f, iterations=%d, %d rejections in a row",
        reject_reason.c_str(), result.nearest_voxel_transformation_likelihood,
        result.iteration_num, consecutive_rejects_);
    }
    publishMapToOdom(stamp);
  }

  PointCloud::Ptr preprocess(
    const sensor_msgs::msg::PointCloud2 & msg, const rclcpp::Time & stamp,
    const Eigen::Isometry3d & base_to_sensor, const Eigen::Isometry3d & odom_to_base)
  {
    const bool has_time = hasField(msg, "time", sensor_msgs::msg::PointField::FLOAT32);
    const size_t num_points = static_cast<size_t>(msg.width) * msg.height;

    std::vector<Eigen::Vector3d> points;
    std::vector<float> times;
    points.reserve(num_points);
    times.reserve(num_points);

    const double min_r2 = min_scan_range_ * min_scan_range_;
    const double max_r2 = max_scan_range_ * max_scan_range_;
    sensor_msgs::PointCloud2ConstIterator<float> it_x(msg, "x");
    sensor_msgs::PointCloud2ConstIterator<float> it_y(msg, "y");
    sensor_msgs::PointCloud2ConstIterator<float> it_z(msg, "z");
    std::unique_ptr<sensor_msgs::PointCloud2ConstIterator<float>> it_t;
    if (has_time) {
      it_t = std::make_unique<sensor_msgs::PointCloud2ConstIterator<float>>(msg, "time");
    }

    float earliest = 0.0F;
    for (size_t i = 0; i < num_points; ++i, ++it_x, ++it_y, ++it_z) {
      const float t = has_time ? **it_t : 0.0F;
      if (has_time) {
        ++(*it_t);
      }
      const Eigen::Vector3d p(*it_x, *it_y, *it_z);
      if (!p.allFinite()) {
        continue;
      }
      const double r2 = p.squaredNorm();
      if (r2 < min_r2 || r2 > max_r2) {
        continue;
      }
      points.push_back(p);
      times.push_back(t);
      earliest = std::min(earliest, t);
    }

    const double window = -static_cast<double>(earliest);
    Eigen::Quaterniond q_start = Eigen::Quaterniond::Identity();
    Eigen::Vector3d t_start = Eigen::Vector3d::Zero();
    const bool do_deskew = deskew_ && has_time && window > 1e-3 &&
                           estimateScanMotion(stamp, window, odom_to_base, q_start, t_start);
    if (deskew_ && !has_time) {
      RCLCPP_WARN_ONCE(get_logger(), "Point cloud has no float32 'time' field; deskew disabled");
    }

    auto cloud = std::make_shared<PointCloud>();
    cloud->reserve(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
      Eigen::Vector3d p = base_to_sensor * points[i];
      if (do_deskew) {
        // s = 1 at the first point of the scan, 0 at the header stamp.
        const double s = -static_cast<double>(times[i]) / window;
        p = Eigen::Quaterniond::Identity().slerp(s, q_start) * p + s * t_start;
      }
      if (p.z() < min_height_ || p.z() > max_height_) {
        continue;
      }
      cloud->push_back(pcl::PointXYZ(
        static_cast<float>(p.x()), static_cast<float>(p.y()), static_cast<float>(p.z())));
    }
    return voxelDownsample(cloud, source_voxel_leaf_size_);
  }

  // Pose of base_link at (stamp - window) expressed in base_link at stamp.
  bool estimateScanMotion(
    const rclcpp::Time & stamp, double window, const Eigen::Isometry3d & odom_to_base_end,
    Eigen::Quaterniond & q_start, Eigen::Vector3d & t_start)
  {
    const rclcpp::Time start = stamp - rclcpp::Duration::from_seconds(window);
    Eigen::Isometry3d odom_to_base_start;
    if (!lookupOdomToBase(start, 0.0, false, odom_to_base_start)) {
      return false;
    }
    const Eigen::Isometry3d end_to_start = odom_to_base_end.inverse() * odom_to_base_start;
    t_start = end_to_start.translation();
    q_start = Eigen::Quaterniond(end_to_start.linear());

    Eigen::Vector3d omega;
    if (deskew_use_imu_ && meanAngularVelocityInBase(start, stamp, window, omega)) {
      const Eigen::Vector3d rotvec = -omega * window;
      const double angle = rotvec.norm();
      q_start = angle > 1e-9 ? Eigen::Quaterniond(Eigen::AngleAxisd(angle, rotvec / angle))
                             : Eigen::Quaterniond::Identity();
    }
    return true;
  }

  bool meanAngularVelocityInBase(
    const rclcpp::Time & start, const rclcpp::Time & end, double window, Eigen::Vector3d & omega)
  {
    std::string frame;
    Eigen::Vector3d sum = Eigen::Vector3d::Zero();
    int count = 0;
    {
      std::lock_guard<std::mutex> lock(imu_mutex_);
      frame = imu_frame_;
      for (const auto & sample : imu_buffer_) {
        if (sample.stamp >= start && sample.stamp <= end) {
          sum += sample.angular_velocity;
          ++count;
        }
      }
    }
    if (count < std::max(3, static_cast<int>(window * kMinImuRateHz))) {
      return false;
    }
    Eigen::Isometry3d base_to_imu;
    if (!lookupStaticFromBase(frame, base_to_imu)) {
      return false;
    }
    omega = base_to_imu.linear() * (sum / count);
    return true;
  }

  void accumulate(const PointCloud::Ptr & frame, const Eigen::Isometry3d & odom_to_base)
  {
    auto in_odom = std::make_shared<PointCloud>();
    pcl::transformPointCloud(*frame, *in_odom, toAffine3f(odom_to_base));
    scan_history_.push_back(in_odom);
    while (static_cast<int>(scan_history_.size()) > scan_accumulation_count_) {
      scan_history_.pop_front();
    }
  }

  PointCloud::Ptr mergedScan(const Eigen::Isometry3d & odom_to_base)
  {
    auto merged = std::make_shared<PointCloud>();
    const Eigen::Affine3f base_to_odom = toAffine3f(odom_to_base.inverse());
    for (const auto & cloud : scan_history_) {
      PointCloud in_base;
      pcl::transformPointCloud(*cloud, in_base, base_to_odom);
      *merged += in_base;
    }
    if (scan_history_.size() > 1) {
      return voxelDownsample(merged, source_voxel_leaf_size_);
    }
    return merged;
  }

  std::string validate(
    const pclomp::NdtResult & result, const Eigen::Isometry3d & guess,
    const Eigen::Isometry3d & estimate) const
  {
    if (!ndt_->hasConverged()) {
      return "not converged";
    }
    if (result.nearest_voxel_transformation_likelihood < nvtl_threshold_) {
      return "low NVTL";
    }
    if (!skip_jump_check_ && consecutive_rejects_ < kJumpCheckMaxRejects) {
      const Eigen::Isometry3d delta = guess.inverse() * estimate;
      if (delta.translation().norm() > max_translation_jump_) {
        return "translation jump";
      }
      if (std::abs(yawOf(delta)) > max_yaw_jump_) {
        return "yaw jump";
      }
    }
    return "";
  }

  bool lookupStaticFromBase(const std::string & frame, Eigen::Isometry3d & base_to_frame)
  {
    const auto cached = static_transforms_.find(frame);
    if (cached != static_transforms_.end()) {
      base_to_frame = cached->second;
      return true;
    }
    try {
      const auto tf = tf_buffer_->lookupTransform(base_frame_, frame, tf2::TimePointZero);
      base_to_frame = tf2::transformToEigen(tf.transform);
      static_transforms_.emplace(frame, base_to_frame);
      return true;
    } catch (const tf2::TransformException & ex) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 3000, "TF %s -> %s: %s", base_frame_.c_str(), frame.c_str(),
        ex.what());
      return false;
    }
  }

  bool lookupOdomToBase(
    const rclcpp::Time & stamp, double timeout_sec, bool warn, Eigen::Isometry3d & odom_to_base)
  {
    try {
      const auto tf = tf_buffer_->lookupTransform(
        odom_frame_, base_frame_, stamp, rclcpp::Duration::from_seconds(timeout_sec));
      odom_to_base = tf2::transformToEigen(tf.transform);
      return true;
    } catch (const tf2::TransformException & ex) {
      if (warn) {
        RCLCPP_WARN_THROTTLE(
          get_logger(), *get_clock(), 3000, "TF %s -> %s: %s (need wheel odometry TF)",
          odom_frame_.c_str(), base_frame_.c_str(), ex.what());
      }
      return false;
    }
  }

  void publishDiagnostics(const pclomp::NdtResult & result)
  {
    std_msgs::msg::Float32 nvtl;
    nvtl.data = result.nearest_voxel_transformation_likelihood;
    nvtl_pub_->publish(nvtl);
    std_msgs::msg::Int32 iterations;
    iterations.data = result.iteration_num;
    iteration_pub_->publish(iterations);
  }

  void publishEstimate(
    const rclcpp::Time & stamp, const Eigen::Isometry3d & map_to_base,
    const Eigen::Matrix<double, 6, 6> & hessian)
  {
    const std::array<double, 36> covariance = covarianceFromHessian(hessian);

    geometry_msgs::msg::PoseWithCovarianceStamped pose_msg;
    pose_msg.header.stamp = stamp;
    pose_msg.header.frame_id = map_frame_;
    pose_msg.pose.pose = isometryToPoseMsg(map_to_base);
    pose_msg.pose.covariance = covariance;
    pose_pub_->publish(pose_msg);

    nav_msgs::msg::Odometry localization_odom;
    localization_odom.header = pose_msg.header;
    localization_odom.child_frame_id = odom_frame_;
    localization_odom.pose.pose = isometryToPoseMsg(map_to_odom_);
    localization_odom.pose.covariance = covariance;
    odom_pub_->publish(localization_odom);
  }

  void publishAligned(const rclcpp::Time & stamp, const PointCloud::Ptr & aligned)
  {
    if (aligned_pub_->get_subscription_count() == 0) {
      return;
    }
    sensor_msgs::msg::PointCloud2 msg;
    pcl::toROSMsg(*aligned, msg);
    msg.header.stamp = stamp;
    msg.header.frame_id = map_frame_;
    aligned_pub_->publish(msg);
  }

  void publishMapToOdom(const rclcpp::Time & stamp)
  {
    if (!publish_tf_ || !has_pose_) {
      return;
    }
    geometry_msgs::msg::TransformStamped tf = tf2::eigenToTransform(map_to_odom_);
    tf.header.stamp = stamp;
    tf.header.frame_id = map_frame_;
    tf.child_frame_id = odom_frame_;
    tf_broadcaster_->sendTransform(tf);
  }

  std::string map_pcd_path_;
  std::string map_frame_;
  std::string odom_frame_;
  std::string base_frame_;

  double ndt_resolution_;
  double ndt_step_size_;
  double ndt_transformation_epsilon_;
  int ndt_max_iterations_;
  int ndt_num_threads_;
  int ndt_search_method_;
  float source_voxel_leaf_size_;
  float map_voxel_leaf_size_;
  double min_scan_range_;
  double max_scan_range_;
  double min_height_;
  double max_height_;

  bool deskew_;
  bool deskew_use_imu_;
  int scan_accumulation_count_;

  double nvtl_threshold_;
  double max_translation_jump_;
  double max_yaw_jump_;
  bool publish_tf_;

  PointCloud::Ptr map_cloud_;
  std::shared_ptr<pclomp::NormalDistributionsTransform<pcl::PointXYZ, pcl::PointXYZ>> ndt_;

  Eigen::Isometry3d map_to_odom_{Eigen::Isometry3d::Identity()};
  Eigen::Isometry3d initial_map_to_base_{Eigen::Isometry3d::Identity()};
  bool has_pose_{false};
  bool pending_initial_pose_{false};
  bool skip_jump_check_{false};
  int consecutive_rejects_{0};

  std::deque<PointCloud::Ptr> scan_history_;
  std::map<std::string, Eigen::Isometry3d> static_transforms_;

  std::mutex imu_mutex_;
  std::deque<GyroSample> imu_buffer_;
  std::string imu_frame_;

  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;

  rclcpp::CallbackGroup::SharedPtr imu_group_;
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr cloud_sub_;
  rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
  rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr initial_pose_sub_;
  rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr pose_pub_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr nvtl_pub_;
  rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr iteration_pub_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr aligned_pub_;
};

}  // namespace ndt_localizer

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ndt_localizer::NdtLocalizerNode>();
  // The IMU callback group runs on its own thread so gyro samples keep arriving during NDT.
  rclcpp::executors::MultiThreadedExecutor executor(rclcpp::ExecutorOptions(), 2);
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
  return 0;
}
