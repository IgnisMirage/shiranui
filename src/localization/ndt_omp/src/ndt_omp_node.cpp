/**
 * @file ndt_omp_node.cpp
 * @brief scan-to-map NDT that owns exactly one transform: map -> odom.
 *
 * ROS 2 port of ia-amr-ros/src/localization/ndt_omp (ndt_omp_node). The per-scan flow, the
 * gates and the parameter names are unchanged; see README.md in this package for the
 * division of labour with the rest of the TF tree and the design constraints behind the gates.
 */

#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>

#include <rclcpp/rclcpp.hpp>

#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

#include <pcl/common/transforms.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/search/kdtree.h>
#include <pcl_conversions/pcl_conversions.h>

#include <tf2_eigen/tf2_eigen.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>

#include <ndt_omp/msg/ndt_status.hpp>
#include <ndt_omp/ndt_scan_matcher.hpp>

namespace {

using PointT = pcl::PointXYZI;
using Matcher = ndt_omp::NdtScanMatcher<PointT, PointT>;
using NdtStatus = ndt_omp::msg::NdtStatus;

/// NDT returns a plain 4x4; re-orthonormalise before treating it as a rigid transform.
Eigen::Isometry3f toIsometry(const Eigen::Matrix4f& m) {
  Eigen::Isometry3f iso = Eigen::Isometry3f::Identity();
  iso.linear() = Eigen::Quaternionf(Eigen::Matrix3f(m.block<3, 3>(0, 0))).normalized().toRotationMatrix();
  iso.translation() = m.block<3, 1>(0, 3);
  return iso;
}

geometry_msgs::msg::Pose toPoseMsg(const Eigen::Isometry3f& t) {
  const Eigen::Quaternionf q = Eigen::Quaternionf(t.linear()).normalized();
  geometry_msgs::msg::Pose pose;
  pose.position.x = t.translation().x();
  pose.position.y = t.translation().y();
  pose.position.z = t.translation().z();
  pose.orientation.x = q.x();
  pose.orientation.y = q.y();
  pose.orientation.z = q.z();
  pose.orientation.w = q.w();
  return pose;
}

geometry_msgs::msg::Transform toTransformMsg(const Eigen::Isometry3f& t) {
  const Eigen::Quaternionf q = Eigen::Quaternionf(t.linear()).normalized();
  geometry_msgs::msg::Transform tr;
  tr.translation.x = t.translation().x();
  tr.translation.y = t.translation().y();
  tr.translation.z = t.translation().z();
  tr.rotation.x = q.x();
  tr.rotation.y = q.y();
  tr.rotation.z = q.z();
  tr.rotation.w = q.w();
  return tr;
}

double rotationAngle(const Eigen::Isometry3f& t) {
  return std::abs(static_cast<double>(Eigen::AngleAxisf(t.linear()).angle()));
}

}  // namespace

class NdtOmpLocalizer : public rclcpp::Node {
public:
  NdtOmpLocalizer() : Node("ndt_omp_node") {
    loadParams();

    tf_buffer_ = std::make_shared<tf2_ros::Buffer>(get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);
    tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

    downsample_filter_.setLeafSize(downsample_resolution_, downsample_resolution_, downsample_resolution_);

    createMatcher();

    odom_pub_ = create_publisher<nav_msgs::msg::Odometry>(odom_topic_, 5);
    status_pub_ = create_publisher<NdtStatus>("~/status", 5);
    if (!aligned_points_topic_.empty()) {
      aligned_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>(aligned_points_topic_, 1);
    }
    if (publishGlobalmapView()) {
      globalmap_view_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>(
          globalmap_view_topic_, rclcpp::QoS(1).transient_local());
    }

    // The map goes in before any scan can arrive; a 1 GB PCD takes tens of seconds.
    loadGlobalmapFromFile();

    if (specify_init_pose_) {
      // Applied on the first scan that has an odom -> base_link TF: map -> odom needs
      // both halves, and at startup the odometry has usually not arrived yet.
      pending_map_to_base_ = init_map_to_base_;
      have_pending_init_ = true;
      RCLCPP_INFO(get_logger(), "initial pose taken from parameters (pose of %s in %s)",
                  base_link_frame_id_.c_str(), map_frame_id_.c_str());
    } else {
      RCLCPP_INFO(get_logger(), "waiting for %s before localizing", initialpose_topic_.c_str());
    }

    // Separate callback groups (with a two-thread executor) so an initial pose still lands
    // while NDT is running; state_mutex_ keeps the scan path itself serial.
    rclcpp::SubscriptionOptions initialpose_options;
    initialpose_options.callback_group =
        create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    initialpose_sub_ = create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
        initialpose_topic_, 1,
        [this](const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg) {
          initialposeCallback(msg);
        },
        initialpose_options);

    rclcpp::SubscriptionOptions points_options;
    points_options.callback_group =
        create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    points_sub_ = create_subscription<sensor_msgs::msg::PointCloud2>(
        points_topic_, rclcpp::SensorDataQoS().keep_last(points_queue_size_),
        [this](const sensor_msgs::msg::PointCloud2::SharedPtr msg) { pointsCallback(msg); },
        points_options);

    RCLCPP_INFO(get_logger(), "ndt_omp_node up: points=%s -> %s, publishing %s -> %s only",
                points_topic_.c_str(), odom_topic_.c_str(), map_frame_id_.c_str(),
                robot_odom_frame_id_.c_str());
  }

private:
  // ------------------------------------------------------------------ parameters

  void loadParams() {
    points_topic_ = declare_parameter<std::string>("points_topic", "/mid360/points");
    initialpose_topic_ = declare_parameter<std::string>("initialpose_topic", "/initialpose");
    // Pose of base_frame_id in the map, published like the other localizers' output.
    odom_topic_ = declare_parameter<std::string>("odom_topic", "/localization/odom");
    aligned_points_topic_ = declare_parameter<std::string>("aligned_points_topic", "/aligned_points");
    globalmap_view_topic_ = declare_parameter<std::string>("globalmap_view_topic", "/globalmap_view");
    map_frame_id_ = declare_parameter<std::string>("global_frame_id", "map");
    robot_odom_frame_id_ = declare_parameter<std::string>("odom_frame_id", "odom");
    base_link_frame_id_ = declare_parameter<std::string>("base_frame_id", "base_link");

    globalmap_pcd_ = declare_parameter<std::string>("globalmap_pcd", "");
    view_downsample_resolution_ = declare_parameter<double>("view_downsample_resolution", 0.5);

    ndt_resolution_ = declare_parameter<double>("ndt_resolution", 0.5);
    ndt_neighbor_search_method_ = declare_parameter<std::string>("ndt_neighbor_search_method", "DIRECT7");
    ndt_num_threads_ = declare_parameter<int>("ndt_num_threads", 0);
    ndt_max_iterations_ = declare_parameter<int>("ndt_max_iterations", 30);
    ndt_transformation_epsilon_ = declare_parameter<double>("ndt_transformation_epsilon", 0.01);
    ndt_step_size_ = declare_parameter<double>("ndt_step_size", 0.1);
    downsample_resolution_ = declare_parameter<double>("downsample_resolution", 0.1);
    min_scan_points_ = declare_parameter<int>("min_scan_points", 100);

    specify_init_pose_ = declare_parameter<bool>("specify_init_pose", false);
    const double px = declare_parameter<double>("init_pos_x", 0.0);
    const double py = declare_parameter<double>("init_pos_y", 0.0);
    const double pz = declare_parameter<double>("init_pos_z", 0.0);
    const double qx = declare_parameter<double>("init_ori_x", 0.0);
    const double qy = declare_parameter<double>("init_ori_y", 0.0);
    const double qz = declare_parameter<double>("init_ori_z", 0.0);
    const double qw = declare_parameter<double>("init_ori_w", 1.0);
    init_map_to_base_ = Eigen::Isometry3f::Identity();
    init_map_to_base_.linear() =
        Eigen::Quaternionf(static_cast<float>(qw), static_cast<float>(qx), static_cast<float>(qy),
                           static_cast<float>(qz))
            .normalized()
            .toRotationMatrix();
    init_map_to_base_.translation() =
        Eigen::Vector3f(static_cast<float>(px), static_cast<float>(py), static_cast<float>(pz));

    max_linear_speed_skip_ = declare_parameter<double>("max_linear_speed_skip", 0.0);

    fitness_max_ = declare_parameter<double>("fitness_max", 1.0);
    min_inlier_ratio_ = declare_parameter<double>("min_inlier_ratio", 0.0);
    min_trans_probability_ = declare_parameter<double>("min_trans_probability", 0.0);
    max_step_trans_ = declare_parameter<double>("max_step_trans", 1.0);
    max_step_rot_ = declare_parameter<double>("max_step_rot", 15.0) * M_PI / 180.0;

    publish_tf_ = declare_parameter<bool>("publish_tf", true);
    tf_timeout_ = declare_parameter<double>("tf_timeout", 0.1);
    transform_tolerance_ = declare_parameter<double>("transform_tolerance", 0.3);
    // 1 keeps the freshest scan when NDT cannot keep up (the right thing on the vehicle);
    // a bag replay wants a backlog instead so every scan is evaluated.
    points_queue_size_ = declare_parameter<int>("points_queue_size", 1);
  }

  bool publishGlobalmapView() const { return !globalmap_view_topic_.empty(); }

  // ------------------------------------------------------------------ setup

  void createMatcher() {
    ndt_ = std::make_unique<Matcher>();
    ndt_->setResolution(static_cast<float>(ndt_resolution_));
    ndt_->setTransformationEpsilon(ndt_transformation_epsilon_);
    ndt_->setMaximumIterations(ndt_max_iterations_);
    ndt_->setStepSize(ndt_step_size_);
    if (ndt_num_threads_ > 0) {
      ndt_->setNumThreads(ndt_num_threads_);
    }

    if (ndt_neighbor_search_method_ == "DIRECT1") {
      ndt_->setNeighborhoodSearchMethod(pclomp::DIRECT1);
    } else if (ndt_neighbor_search_method_ == "DIRECT7") {
      ndt_->setNeighborhoodSearchMethod(pclomp::DIRECT7);
    } else if (ndt_neighbor_search_method_ == "KDTREE") {
      ndt_->setNeighborhoodSearchMethod(pclomp::KDTREE);
    } else {
      RCLCPP_WARN(get_logger(), "unknown ndt_neighbor_search_method '%s'; using DIRECT7",
                  ndt_neighbor_search_method_.c_str());
      ndt_->setNeighborhoodSearchMethod(pclomp::DIRECT7);
    }
    RCLCPP_INFO(get_logger(), "NDT: resolution=%.2f search=%s max_iterations=%d", ndt_resolution_,
                ndt_neighbor_search_method_.c_str(), ndt_max_iterations_);

    // pclomp's NDT works off its own voxel grid and never touches the target kd-tree, but
    // pcl::Registration::initCompute() would still build one over the whole map on the
    // first align (seconds to minutes, and a second copy of the map in RAM). Hand it an
    // empty tree with force_no_recompute so that never happens. Consequence:
    // getFitnessScore() would return garbage -- match quality comes from the voxel grid,
    // see NdtScanMatcher::computeMatchStats().
    pcl::search::KdTree<PointT>::Ptr unused_tree(new pcl::search::KdTree<PointT>());
    ndt_->setSearchMethodTarget(unused_tree, true);
  }

  void loadGlobalmapFromFile() {
    if (globalmap_pcd_.empty()) {
      RCLCPP_ERROR(get_logger(), "globalmap_pcd is not set; this node cannot localize");
      return;
    }

    RCLCPP_INFO(get_logger(), "loading globalmap from %s", globalmap_pcd_.c_str());
    pcl::PointCloud<PointT>::Ptr cloud(new pcl::PointCloud<PointT>());
    if (pcl::io::loadPCDFile(globalmap_pcd_, *cloud) < 0) {
      RCLCPP_ERROR(get_logger(), "failed to load globalmap from %s", globalmap_pcd_.c_str());
      return;
    }

    setGlobalmap(cloud);
  }

  void setGlobalmap(const pcl::PointCloud<PointT>::Ptr& cloud) {
    if (cloud->empty()) {
      RCLCPP_ERROR(get_logger(), "refusing an empty globalmap");
      return;
    }
    cloud->header.frame_id = map_frame_id_;

    std::lock_guard<std::mutex> lock(state_mutex_);
    globalmap_ = cloud;
    // Target voxelization is the expensive part; it happens here and nowhere else.
    ndt_->setInputTarget(globalmap_);
    RCLCPP_INFO(get_logger(), "globalmap ready: %zu points (NDT target built)", globalmap_->size());

    publishGlobalmapViewLocked();
  }

  void publishGlobalmapViewLocked() {
    if (!globalmap_ || !publishGlobalmapView()) {
      return;
    }
    pcl::PointCloud<PointT>::Ptr view(globalmap_);
    if (view_downsample_resolution_ > 0.0) {
      pcl::VoxelGrid<PointT> voxelgrid;
      const float leaf = static_cast<float>(view_downsample_resolution_);
      voxelgrid.setLeafSize(leaf, leaf, leaf);
      voxelgrid.setInputCloud(globalmap_);
      view.reset(new pcl::PointCloud<PointT>());
      voxelgrid.filter(*view);
      view->header = globalmap_->header;
    }
    RCLCPP_INFO(get_logger(), "publishing %s: %zu points (leaf %.2f m)", globalmap_view_topic_.c_str(),
                view->size(), view_downsample_resolution_);
    sensor_msgs::msg::PointCloud2 msg;
    pcl::toROSMsg(*view, msg);
    msg.header.frame_id = map_frame_id_;
    msg.header.stamp = now();
    globalmap_view_pub_->publish(msg);
  }

  // ------------------------------------------------------------------ callbacks

  /**
   * @brief /initialpose is the pose of base_frame_id in the map.
   *
   * map -> odom needs the odom -> base_link half as well, so the pose is only stashed here
   * and consumed by the next scan, which has a matching odometry lookup.
   */
  void initialposeCallback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg) {
    if (!msg->header.frame_id.empty() && msg->header.frame_id != map_frame_id_) {
      RCLCPP_WARN(get_logger(), "initialpose frame_id is %s, expected %s; using it as-is",
                  msg->header.frame_id.c_str(), map_frame_id_.c_str());
    }
    const auto& p = msg->pose.pose.position;
    const auto& q = msg->pose.pose.orientation;

    Eigen::Isometry3f map_to_base = Eigen::Isometry3f::Identity();
    map_to_base.linear() = Eigen::Quaternionf(static_cast<float>(q.w), static_cast<float>(q.x),
                                              static_cast<float>(q.y), static_cast<float>(q.z))
                               .normalized()
                               .toRotationMatrix();
    map_to_base.translation() = Eigen::Vector3f(static_cast<float>(p.x), static_cast<float>(p.y),
                                                static_cast<float>(p.z));

    std::lock_guard<std::mutex> lock(state_mutex_);
    pending_map_to_base_ = map_to_base;
    have_pending_init_ = true;
    RCLCPP_INFO(get_logger(), "initial pose received: %s at (%.3f, %.3f, %.3f) in %s",
                base_link_frame_id_.c_str(), p.x, p.y, p.z, map_frame_id_.c_str());
  }

  void pointsCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg) {
    // One NDT at a time, and no reinitialisation halfway through one.
    std::lock_guard<std::mutex> lock(state_mutex_);

    const rclcpp::Time stamp(msg->header.stamp, RCL_ROS_TIME);
    if (!globalmap_) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "no globalmap yet; dropping scans");
      return;
    }

    pcl::PointCloud<PointT>::Ptr raw(new pcl::PointCloud<PointT>());
    pcl::fromROSMsg(*msg, *raw);
    if (raw->empty()) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "empty cloud on %s",
                           points_topic_.c_str());
      return;
    }

    // 1. into base_link. The mounting tilt lives in TF, so this is where it is applied --
    //    the driver must not pre-rotate the cloud.
    Eigen::Isometry3f T_base_sensor;
    try {
      const auto tf = tf_buffer_->lookupTransform(base_link_frame_id_, msg->header.frame_id, stamp,
                                                  rclcpp::Duration::from_seconds(tf_timeout_));
      T_base_sensor = tf2::transformToEigen(tf).cast<float>();
    } catch (const tf2::TransformException& ex) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "no TF %s -> %s at %.3f: %s",
                           base_link_frame_id_.c_str(), msg->header.frame_id.c_str(),
                           stamp.seconds(), ex.what());
      return;
    }
    pcl::PointCloud<PointT>::Ptr cloud(new pcl::PointCloud<PointT>());
    pcl::transformPointCloud(*raw, *cloud, T_base_sensor.matrix());
    cloud->header.frame_id = base_link_frame_id_;

    // 2. downsample. Too few points (an occluded sensor) still goes through the odometry
    //    lookup so the scan can be held below: dropping it silently would stop map -> odom,
    //    /localization/odom and status all at once.
    pcl::PointCloud<PointT>::Ptr filtered = downsample(cloud);
    const bool too_few_points = static_cast<int>(filtered->size()) < min_scan_points_;
    if (too_few_points) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
                           "only %zu points after downsampling (< %d); holding", filtered->size(),
                           min_scan_points_);
    }

    // 3. odom -> base_link at the scan stamp. Never fabricated: without it there is no
    //    way to express the result as map -> odom, so the scan is dropped.
    Eigen::Isometry3f T_odom_base;
    if (!lookupOdomBase(stamp, &T_odom_base)) {
      NdtStatus status;
      status.header = msg->header;
      status.reason = "no_odom_tf";
      status.num_points = static_cast<int>(filtered->size());
      status_pub_->publish(status);
      return;
    }

    const double linear_speed = updateOdomRates(stamp, T_odom_base);

    // 4. consume a pending initial pose now that both halves are known
    if (have_pending_init_) {
      T_map_odom_ = pending_map_to_base_ * T_odom_base.inverse();
      have_pending_init_ = false;
      initialized_ = true;
      step_gate_suspended_ = true;
      const Eigen::Vector3f t = T_map_odom_.translation();
      RCLCPP_INFO(get_logger(), "initialized: %s -> %s = (%.3f, %.3f, %.3f)", map_frame_id_.c_str(),
                  robot_odom_frame_id_.c_str(), t.x(), t.y(), t.z());
    }
    if (!initialized_) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "not initialized; waiting for %s",
                           initialpose_topic_.c_str());
      return;
    }

    // map-frame prediction of base_link: the last accepted map -> odom composed with the
    // current odom -> base_link, i.e. dead reckoning on the wheels since the last accept.
    const Eigen::Isometry3f T_guess = T_map_odom_ * T_odom_base;

    NdtStatus status;
    status.header = msg->header;
    status.num_points = static_cast<int>(filtered->size());
    status.linear_speed = static_cast<float>(linear_speed);

    // Reported only: a turn no longer skips NDT, gate B judges the match instead.
    if (have_odom_yaw_rate_) {
      status.yaw_rate = static_cast<float>(odom_yaw_rate_);
    }

    if (too_few_points) {
      hold(stamp, T_guess, "too_few_points", &status);
      return;
    }

    // ---- gate A: do not run NDT above a speed the guess cannot keep up with ----
    if (max_linear_speed_skip_ > 0.0 && linear_speed > max_linear_speed_skip_) {
      RCLCPP_DEBUG(get_logger(), "gate A: linear speed %.3f m/s (limit %.3f)", linear_speed,
                   max_linear_speed_skip_);
      hold(stamp, T_guess, "linear_speed", &status);
      return;
    }

    // ---- NDT --------------------------------------------------------------------
    const auto t_start = std::chrono::steady_clock::now();
    pcl::PointCloud<PointT>::Ptr aligned(new pcl::PointCloud<PointT>());
    ndt_->setInputSource(filtered);
    ndt_->align(*aligned, T_guess.matrix());

    // align() had base_link as the source frame, so this is map <- base_link directly.
    const Eigen::Isometry3f T_map_base = toIsometry(ndt_->getFinalTransformation());
    const Matcher::MatchStats stats = ndt_->computeMatchStats(*aligned);
    const Eigen::Isometry3f step = T_guess.inverse() * T_map_base;
    const double step_trans = step.translation().norm();
    const double step_rot = rotationAngle(step);

    status.ndt_ran = true;
    status.has_converged = ndt_->hasConverged();
    status.iterations = ndt_->getFinalNumIteration();
    status.num_matched = stats.matched;
    status.inlier_ratio = static_cast<float>(stats.inlier_ratio);
    status.fitness = static_cast<float>(stats.fitness);
    status.trans_probability = static_cast<float>(ndt_->getTransformationProbability());
    status.step_trans = static_cast<float>(step_trans);
    status.step_rot = static_cast<float>(step_rot * 180.0 / M_PI);
    status.processing_time = static_cast<float>(
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t_start).count());

    // Published whether or not the gate accepts: seeing where a rejected scan landed is
    // the point of looking at this topic.
    if (aligned_pub_ && aligned_pub_->get_subscription_count() > 0) {
      sensor_msgs::msg::PointCloud2 aligned_msg;
      pcl::toROSMsg(*aligned, aligned_msg);
      aligned_msg.header.frame_id = map_frame_id_;
      aligned_msg.header.stamp = msg->header.stamp;
      aligned_pub_->publish(aligned_msg);
    }

    // ---- gate B: a converged match can still be the wrong one -------------------
    std::string reason;
    if (!ndt_->hasConverged()) {
      reason = "not_converged";
    } else if (stats.fitness > fitness_max_) {
      reason = "fitness";
    } else if (stats.inlier_ratio < min_inlier_ratio_) {
      reason = "inlier_ratio";
    } else if (status.trans_probability < min_trans_probability_) {
      reason = "trans_probability";
    } else if (!step_gate_suspended_ && step_trans > max_step_trans_) {
      reason = "step_trans";
    } else if (!step_gate_suspended_ && step_rot > max_step_rot_) {
      reason = "step_rot";
    }

    if (!reason.empty()) {
      RCLCPP_INFO_THROTTLE(get_logger(), *get_clock(), 1000,
                           "gate B rejected (%s): fitness=%.3f inliers=%.2f prob=%.2f "
                           "step=%.3f m / %.2f deg",
                           reason.c_str(), stats.fitness, stats.inlier_ratio,
                           static_cast<double>(status.trans_probability), step_trans,
                           step_rot * 180.0 / M_PI);
      hold(stamp, T_guess, reason, &status);
      return;
    }

    // ---- accept -----------------------------------------------------------------
    T_map_odom_ = T_map_base * T_odom_base.inverse();
    step_gate_suspended_ = false;
    status.accepted = true;
    publish(stamp, T_map_base, status);
  }

  // ------------------------------------------------------------------ helpers

  pcl::PointCloud<PointT>::Ptr downsample(const pcl::PointCloud<PointT>::Ptr& cloud) {
    if (downsample_resolution_ <= 0.0) {
      return cloud;
    }
    pcl::PointCloud<PointT>::Ptr filtered(new pcl::PointCloud<PointT>());
    downsample_filter_.setInputCloud(cloud);
    downsample_filter_.filter(*filtered);
    filtered->header = cloud->header;
    return filtered;
  }

  bool lookupOdomBase(const rclcpp::Time& stamp, Eigen::Isometry3f* out) {
    try {
      const auto tf = tf_buffer_->lookupTransform(robot_odom_frame_id_, base_link_frame_id_, stamp,
                                                  rclcpp::Duration::from_seconds(tf_timeout_));
      *out = tf2::transformToEigen(tf).cast<float>();
      return true;
    } catch (const tf2::TransformException& ex) {
      // Deliberately no Time(0) fallback: the latest odometry paired with an older scan is
      // exactly what makes map -> odom jump during a turn.
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
                           "no %s -> %s at the scan stamp %.3f (%s); dropping the scan",
                           robot_odom_frame_id_.c_str(), base_link_frame_id_.c_str(),
                           stamp.seconds(), ex.what());
      return false;
    }
  }

  /// @return linear speed [m/s] from consecutive odometry lookups (0 when not derivable)
  double updateOdomRates(const rclcpp::Time& stamp, const Eigen::Isometry3f& T_odom_base) {
    double linear_speed = 0.0;
    if (have_prev_odom_) {
      const double dt = (stamp - prev_odom_stamp_).seconds();
      if (dt > 1e-3 && dt < 1.0) {
        const Eigen::Isometry3f delta = prev_odom_base_.inverse() * T_odom_base;
        linear_speed = delta.translation().norm() / dt;
        odom_yaw_rate_ = rotationAngle(delta) / dt;
        have_odom_yaw_rate_ = true;
      } else {
        have_odom_yaw_rate_ = false;
      }
    }
    prev_odom_stamp_ = stamp;
    prev_odom_base_ = T_odom_base;
    have_prev_odom_ = true;
    return linear_speed;
  }

  /// Gate rejected this scan: map -> odom is left exactly where it was.
  void hold(const rclcpp::Time& stamp, const Eigen::Isometry3f& T_guess, const std::string& reason,
            NdtStatus* status) {
    status->accepted = false;
    status->reason = reason;
    // T_guess is map -> odom (unchanged) composed with the current odometry, i.e. the pose
    // dead reckons on the wheels while the map anchor holds. Publishing it keeps
    // /localization/odom at the scan rate, and re-stamping the unchanged map -> odom keeps
    // TF lookups alive without moving the anchor.
    publish(stamp, T_guess, *status);
  }

  void publish(const rclcpp::Time& stamp, const Eigen::Isometry3f& T_map_base,
               const NdtStatus& status) {
    // map -> odom is a slowly varying offset, so declare it valid into the near future
    // (amcl/emcl2-style transform_tolerance). Stamped at the scan time it would already be
    // one scan period plus NDT time old on arrival, and latest-time lookups of
    // map -> base_link would be pinned to that instead of following the wheel odometry.
    const rclcpp::Time tf_stamp = stamp + rclcpp::Duration::from_seconds(transform_tolerance_);
    if (publish_tf_ && tf_stamp > last_tf_stamp_) {
      last_tf_stamp_ = tf_stamp;
      geometry_msgs::msg::TransformStamped tf;
      tf.header.stamp = tf_stamp;
      tf.header.frame_id = map_frame_id_;
      tf.child_frame_id = robot_odom_frame_id_;
      tf.transform = toTransformMsg(T_map_odom_);
      tf_broadcaster_->sendTransform(tf);
    }

    nav_msgs::msg::Odometry odom;
    odom.header.stamp = stamp;
    odom.header.frame_id = map_frame_id_;
    odom.child_frame_id = base_link_frame_id_;
    odom.pose.pose = toPoseMsg(T_map_base);
    // Velocity belongs to odom -> base_link, which this node does not own.
    odom_pub_->publish(odom);

    status_pub_->publish(status);
  }

  // ------------------------------------------------------------------ members

  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
  std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;

  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr points_sub_;
  rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr initialpose_sub_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr aligned_pub_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr globalmap_view_pub_;
  rclcpp::Publisher<NdtStatus>::SharedPtr status_pub_;

  std::unique_ptr<Matcher> ndt_;
  pcl::VoxelGrid<PointT> downsample_filter_;
  pcl::PointCloud<PointT>::Ptr globalmap_;

  std::mutex state_mutex_;
  Eigen::Isometry3f T_map_odom_ = Eigen::Isometry3f::Identity();
  bool initialized_ = false;
  // The guess right after an initial pose is only as good as that pose, so a correct match
  // can be far from it; the step gates stay off until the first accept.
  bool step_gate_suspended_ = false;
  bool have_pending_init_ = false;
  Eigen::Isometry3f pending_map_to_base_ = Eigen::Isometry3f::Identity();
  Eigen::Isometry3f init_map_to_base_ = Eigen::Isometry3f::Identity();

  bool have_prev_odom_ = false;
  rclcpp::Time prev_odom_stamp_{0, 0, RCL_ROS_TIME};
  Eigen::Isometry3f prev_odom_base_ = Eigen::Isometry3f::Identity();
  bool have_odom_yaw_rate_ = false;
  double odom_yaw_rate_ = 0.0;

  std::string points_topic_;
  std::string initialpose_topic_;
  std::string odom_topic_;
  std::string aligned_points_topic_;
  std::string globalmap_view_topic_;
  std::string map_frame_id_;
  std::string robot_odom_frame_id_;
  std::string base_link_frame_id_;
  std::string globalmap_pcd_;
  double view_downsample_resolution_ = 0.5;

  double ndt_resolution_ = 0.5;
  std::string ndt_neighbor_search_method_ = "DIRECT7";
  int ndt_num_threads_ = 0;
  int ndt_max_iterations_ = 30;
  double ndt_transformation_epsilon_ = 0.01;
  double ndt_step_size_ = 0.1;
  double downsample_resolution_ = 0.1;
  int min_scan_points_ = 100;

  bool specify_init_pose_ = false;
  double max_linear_speed_skip_ = 0.0;
  double fitness_max_ = 1.0;
  double min_inlier_ratio_ = 0.0;
  double min_trans_probability_ = 0.0;
  double max_step_trans_ = 1.0;
  double max_step_rot_ = 15.0 * M_PI / 180.0;

  bool publish_tf_ = true;
  double tf_timeout_ = 0.1;
  double transform_tolerance_ = 0.3;
  rclcpp::Time last_tf_stamp_{0, 0, RCL_ROS_TIME};
  int points_queue_size_ = 1;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<NdtOmpLocalizer>();
  // Two threads so an initial pose still lands while NDT is running.
  rclcpp::executors::MultiThreadedExecutor executor(rclcpp::ExecutorOptions(), 2);
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
  return 0;
}
