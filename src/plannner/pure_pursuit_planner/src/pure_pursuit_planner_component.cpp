#include "pure_pursuit_planner/pure_pursuit_planner_component.hpp"
#include <angles/angles.h>
#include <cmath>
#include <limits>

using namespace std::chrono_literals;
using std::placeholders::_1;

namespace pure_pursuit_planner
{
PurePursuitNode::PurePursuitNode(const rclcpp::NodeOptions & node_options)
: Node("pure_pursuit_planner", node_options),
  tf_buffer_(this->get_clock()),
  tf_listener_(tf_buffer_)
{
  lookahead_distance_ = declare_parameter<double>("lookahead_distance", 0.5);
  min_lookahead_distance_ = declare_parameter<double>("min_lookahead_distance", 0.3);
  linear_velocity_ = declare_parameter<double>("linear_velocity", 0.8);
  max_angular_velocity_ = declare_parameter<double>("max_angular_velocity", 0.5);
  goal_tolerance_ = declare_parameter<double>("goal_tolerance", 0.05);
  robot_frame_id_ = declare_parameter<std::string>("robot_frame_id", "base_link");
  map_frame_id_ = declare_parameter<std::string>("map_frame_id", "map");
  rotation_threshold_ = declare_parameter<double>("rotation_threshold", 0.1);
  rotation_velocity_ = declare_parameter<double>("rotation_velocity", 0.5);
  max_velocity_ = declare_parameter<double>("max_velocity", 1.0);
  deceleration_ = declare_parameter<double>("deceleration", 0.5);
  acceleration_ = declare_parameter<double>("acceleration", 0.2);
  curvature_safety_factor_ = declare_parameter<double>("curvature_safety_factor", 0.6);
  sensor_delay_ = declare_parameter<double>("sensor_delay", 0.0);
  goal_approach_distance_ = declare_parameter<double>("goal_approach_distance", 0.0);
  goal_approach_speed_ = declare_parameter<double>("goal_approach_speed", 0.05);
  slow_speed_lookahead_threshold_ = declare_parameter<double>("slow_speed_lookahead_threshold", 0.1);
  slow_speed_lookahead_distance_ = declare_parameter<double>("slow_speed_lookahead_distance", 0.35);
  effective_max_speed_ = linear_velocity_;

  // 内部状態の初期化
  follow_path_ = false;
  is_goal_reached_ = false;
  current_state_ = RobotState::STOP;
  
  twist_publisher_ = create_publisher<geometry_msgs::msg::Twist>("/auto_cmd_vel", 10);
  marker_publisher_ = create_publisher<visualization_msgs::msg::MarkerArray>("~/marker", 10);
  mode_publisher_ = create_publisher<std_msgs::msg::String>("~/mode", 10);
  path_subscriber_ = create_subscription<nav_msgs::msg::Path>(
    "path", rclcpp::QoS(10).transient_local().reliable().keep_last(1),
     std::bind(&PurePursuitNode::onPathReceived, this, _1));
  control_timer_ = create_wall_timer(
    10ms, std::bind(&PurePursuitNode::ControlLoop, this));
}

PurePursuitNode::~PurePursuitNode()
{
  if (twist_publisher_) {
    geometry_msgs::msg::Twist stop_cmd;
    stop_cmd.linear.x = 0.0;
    stop_cmd.angular.z = 0.0;
    twist_publisher_->publish(stop_cmd);
  }
}

void PurePursuitNode::CostmapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
{
    costmap_received_ = true;
    costmap_ = *msg;
}

void PurePursuitNode::onPathReceived(const nav_msgs::msg::Path::SharedPtr msg)
{
  if (!msg || msg->poses.empty()) {
    RCLCPP_WARN(get_logger(), "Received empty path, ignoring");
    return;
  }
  if (!isValidPath(*msg)) {
    RCLCPP_WARN(get_logger(), "Received path with NaN/Inf positions, ignoring");
    return;
  }
  current_path_ = msg;
  follow_path_ = true;
  is_goal_reached_ = false;
  closest_idx_ = 0;
  pre_speed_ = 0.0;
  last_omega_ = 0.0;
  computeArcLength();
  updatePathSpeedProfile();
  current_state_ = RobotState::START_ROTATE;
  RCLCPP_INFO(get_logger(), "Path received (%zu poses), starting navigation",
    msg->poses.size());
}

bool PurePursuitNode::isValidPath(const nav_msgs::msg::Path & path) const
{
  for (const auto & pose_stamped : path.poses) {
    const auto & p = pose_stamped.pose.position;
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
      return false;
    }
  }
  return true;
}

void PurePursuitNode::publishStopCmd()
{
  geometry_msgs::msg::Twist cmd_vel{};
  twist_publisher_->publish(cmd_vel);
}

void PurePursuitNode::publishCmdVel(double linear_x, double angular_z)
{
  if (!std::isfinite(linear_x) || !std::isfinite(angular_z)) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
      "Invalid cmd_vel (linear=%.3f, angular=%.3f), publishing stop instead",
      linear_x, angular_z);
    publishStopCmd();
    return;
  }
  geometry_msgs::msg::Twist cmd_vel{};
  cmd_vel.linear.x = linear_x;
  cmd_vel.angular.z = angular_z;
  twist_publisher_->publish(cmd_vel);
}

bool PurePursuitNode::getCurrentPose()
{
  geometry_msgs::msg::TransformStamped transform;
  try {
    transform = tf_buffer_.lookupTransform(
      map_frame_id_, robot_frame_id_, tf2::TimePointZero);
  } catch (tf2::TransformException & ex) {
    RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), 2000,
      "Could not transform %s to %s: %s",
      map_frame_id_.c_str(), robot_frame_id_.c_str(), ex.what());
    pose_valid_ = false;
    return false;
  }
  current_pose_.position.x = transform.transform.translation.x;
  current_pose_.position.y = transform.transform.translation.y;
  current_pose_.position.z = transform.transform.translation.z;
  current_pose_.orientation = transform.transform.rotation;

  if (!std::isfinite(current_pose_.position.x) ||
      !std::isfinite(current_pose_.position.y) ||
      !std::isfinite(tf2::getYaw(current_pose_.orientation)))
  {
    RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), 2000,
      "TF pose contains NaN/Inf (%s -> %s). Reset pose via 2D Pose Estimate.",
      map_frame_id_.c_str(), robot_frame_id_.c_str());
    pose_valid_ = false;
    return false;
  }

  pose_valid_ = true;
  return true;
}

void PurePursuitNode::updateState(){
  if (!current_path_ || current_path_->poses.empty()) {
    return;
  }
  switch (current_state_) {
    case RobotState::STOP:
      if (follow_path_) current_state_ = RobotState::START_ROTATE;
      break;
    case RobotState::START_ROTATE:
      if (isStartAngleReached()) current_state_ = RobotState::FOLLOW;
      
      break;
    case RobotState::FOLLOW:
      if (isGoalReached(current_pose_, current_path_->poses.back())) {
        current_state_ = RobotState::GOAL_ROTATE;
      }
      break;
    case RobotState::GOAL_ROTATE:
      if (isGoalAngleReached()) current_state_ = RobotState::GOAL;
      break;
    case RobotState::GOAL:
      follow_path_ = false;
      is_goal_reached_ = true;
      current_state_ = RobotState::STOP;
      break;
  }
}

std::string PurePursuitNode::getCurrentMode()
{
  switch (current_state_) {
    case RobotState::STOP:
      return "STOP";
    case RobotState::START_ROTATE:
      return "START_ROTATE";
    case RobotState::FOLLOW:
      return "FOLLOW";
    case RobotState::GOAL_ROTATE:
      return "GOAL_ROTATE";
    case RobotState::GOAL:
      return "GOAL";
    default:
      return "UNKNOWN";
  }
}

void PurePursuitNode::ControlLoop()
{
  const bool has_path = current_path_ && !current_path_->poses.empty();
  const bool has_pose = getCurrentPose();

  if (has_path && has_pose) {
    colisionCheck();
    updateState();
  } else if (has_path && !has_pose) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
      "Path is ready but map->%s TF is unavailable; waiting for pose estimate",
      robot_frame_id_.c_str());
  }

  if (!has_path) {
    publishStopCmd();
    std_msgs::msg::String mode_msg;
    mode_msg.data = getCurrentMode();
    mode_publisher_->publish(mode_msg);
    return;
  }

  if (!has_pose) {
    publishStopCmd();
    std_msgs::msg::String mode_msg;
    mode_msg.data = getCurrentMode();
    mode_publisher_->publish(mode_msg);
    return;
  }

  switch (current_state_) {
    case RobotState::STOP:
    case RobotState::GOAL:
      publishStopCmd();
      break;
    case RobotState::START_ROTATE: {
      geometry_msgs::msg::Pose start_target;
      tf2::Quaternion q;
      q.setRPY(0.0, 0.0, getPathStartYaw());
      start_target.orientation = tf2::toMsg(q);
      publishCmdVel(0.0, calculateAngularVelocity(start_target));
      break;
    }
    case RobotState::FOLLOW: {
      const geometry_msgs::msg::Pose control_pose = computeControlPose();
      if (findTargetPoint(control_pose)) {
        detect_velocity(control_pose);
        publishMarkers(target_point_);
      }
      break;
    }
    case RobotState::GOAL_ROTATE: {
      geometry_msgs::msg::Pose goal_target;
      tf2::Quaternion q;
      q.setRPY(0.0, 0.0, getGoalYaw());
      goal_target.orientation = tf2::toMsg(q);
      publishCmdVel(0.0, calculateAngularVelocity(goal_target));
      break;
    }
  }
  std_msgs::msg::String mode_msg;
  mode_msg.data = getCurrentMode();
  mode_publisher_->publish(mode_msg);
}

double PurePursuitNode::calculateAngularVelocity(const geometry_msgs::msg::Pose & target_point)
{
  const double target_yaw = tf2::getYaw(target_point.orientation);
  const double current_yaw = tf2::getYaw(current_pose_.orientation);
  if (!std::isfinite(target_yaw) || !std::isfinite(current_yaw)) {
    return 0.0;
  }

  double angle_diff = angles::shortest_angular_distance(current_yaw, target_yaw);
  if (!std::isfinite(angle_diff)) {
    return 0.0;
  }

  if (std::abs(angle_diff) < rotation_threshold_) {
    return 0.0;
  }

  constexpr double kRotationGain = 2.0;
  const double angular_vel = std::clamp(
    angle_diff * kRotationGain, -rotation_velocity_, rotation_velocity_);
  return std::isfinite(angular_vel) ? angular_vel : 0.0;
}

double PurePursuitNode::calc_dinstanse(
  size_t target_idx, const geometry_msgs::msg::Point & query) const
{
  const auto& p1 = current_path_->poses[target_idx].pose.position;
  const auto& p2 = current_path_->poses[target_idx + 1].pose.position;
  double dx = p2.x - p1.x;
  double dy = p2.y - p1.y;
  double seg_len2 = dx * dx + dy * dy;
  if (seg_len2 < 1e-6) {
    return 0.0;
  }
  double t = ((query.x - p1.x) * dx + (query.y - p1.y) * dy) / seg_len2;
  t = std::clamp(t, 0.0, 1.0);
  const double proj_x = p1.x + t * dx;
  const double proj_y = p1.y + t * dy;
  return std::hypot(query.x - proj_x, query.y - proj_y);
}

void PurePursuitNode::computeArcLength()
{
  cumulative_arc_length_.clear();
  if (!current_path_ || current_path_->poses.empty()) {
    total_path_length_ = 0.0;
    return;
  }

  cumulative_arc_length_.push_back(0.0);
  for (size_t i = 1; i < current_path_->poses.size(); ++i) {
    const auto & p0 = current_path_->poses[i - 1].pose.position;
    const auto & p1 = current_path_->poses[i].pose.position;
    cumulative_arc_length_.push_back(
      cumulative_arc_length_.back() + std::hypot(p1.x - p0.x, p1.y - p0.y));
  }
  total_path_length_ = cumulative_arc_length_.back();
}

bool PurePursuitNode::isGoalApproachEnabled() const
{
  return goal_approach_distance_ > 1e-6 && goal_approach_speed_ > 1e-6;
}

double PurePursuitNode::computeDecelProfileSpeed(double decel_distance) const
{
  if (decel_distance <= 0.0) {
    return 0.0;
  }
  if (deceleration_ <= 1e-6) {
    return linear_velocity_;
  }
  if (isGoalApproachEnabled()) {
    if (decel_distance <= goal_approach_distance_) {
      return goal_approach_speed_;
    }
    const double v_creep_sq = goal_approach_speed_ * goal_approach_speed_;
    return std::sqrt(
      v_creep_sq + 2.0 * deceleration_ * (decel_distance - goal_approach_distance_));
  }
  return std::sqrt(2.0 * deceleration_ * decel_distance);
}

void PurePursuitNode::updatePathSpeedProfile()
{
  const double path_length = total_path_length_;
  double v_max_from_path = 0.0;
  if (deceleration_ > 1e-6 && path_length > goal_tolerance_) {
    v_max_from_path = computeDecelProfileSpeed(path_length - goal_tolerance_);
  }

  if (v_max_from_path < linear_velocity_ && v_max_from_path > 1e-6) {
    effective_max_speed_ = v_max_from_path;
    decel_start_distance_ = path_length;
  } else {
    effective_max_speed_ = linear_velocity_;
    if (deceleration_ > 1e-6) {
      if (isGoalApproachEnabled()) {
        const double v_creep_sq = goal_approach_speed_ * goal_approach_speed_;
        const double v_cruise_sq = linear_velocity_ * linear_velocity_;
        decel_start_distance_ = goal_approach_distance_ +
          std::max(0.0, v_cruise_sq - v_creep_sq) / (2.0 * deceleration_);
      } else {
        decel_start_distance_ = (linear_velocity_ * linear_velocity_) / (2.0 * deceleration_);
      }
    } else {
      decel_start_distance_ = 0.0;
    }
    decel_start_distance_ += goal_tolerance_;
  }
}

double PurePursuitNode::getEffectiveLookaheadDistance() const
{
  if (slow_speed_lookahead_threshold_ > 0.0 &&
      pre_speed_ <= slow_speed_lookahead_threshold_)
  {
    return slow_speed_lookahead_distance_;
  }
  return lookahead_distance_;
}

geometry_msgs::msg::Pose PurePursuitNode::computeControlPose() const
{
  if (sensor_delay_ <= 1e-6) {
    return current_pose_;
  }

  const double yaw = tf2::getYaw(current_pose_.orientation);
  geometry_msgs::msg::Pose predicted = current_pose_;
  predicted.position.x += pre_speed_ * std::cos(yaw) * sensor_delay_;
  predicted.position.y += pre_speed_ * std::sin(yaw) * sensor_delay_;
  tf2::Quaternion q;
  q.setRPY(0.0, 0.0, yaw + last_omega_ * sensor_delay_);
  predicted.orientation = tf2::toMsg(q);
  return predicted;
}

double PurePursuitNode::getRemainingPathDistance(const geometry_msgs::msg::Pose & pose) const
{
  if (!current_path_ || current_path_->poses.empty()) {
    return std::numeric_limits<double>::infinity();
  }

  const auto & current = pose.position;
  if (current_path_->poses.size() == 1) {
    const auto & goal = current_path_->poses.back().pose.position;
    return std::hypot(goal.x - current.x, goal.y - current.y);
  }

  double min_dist = std::numeric_limits<double>::max();
  size_t closest_seg = 0;
  double closest_t = 0.0;
  bool found = false;

  for (size_t i = 0; i + 1 < current_path_->poses.size(); ++i) {
    const auto & p1 = current_path_->poses[i].pose.position;
    const auto & p2 = current_path_->poses[i + 1].pose.position;
    const double dx = p2.x - p1.x;
    const double dy = p2.y - p1.y;
    const double seg_len2 = dx * dx + dy * dy;
    if (seg_len2 < 1e-6) {
      continue;
    }

    double t = ((current.x - p1.x) * dx + (current.y - p1.y) * dy) / seg_len2;
    t = std::clamp(t, 0.0, 1.0);
    const double proj_x = p1.x + t * dx;
    const double proj_y = p1.y + t * dy;
    const double dist = std::hypot(current.x - proj_x, current.y - proj_y);
    if (dist < min_dist) {
      min_dist = dist;
      closest_seg = i;
      closest_t = t;
      found = true;
    }
  }

  if (!found || cumulative_arc_length_.empty()) {
    const auto & goal = current_path_->poses.back().pose.position;
    return std::hypot(goal.x - current.x, goal.y - current.y);
  }

  double s_current = 0.0;
  if (closest_seg + 1 < cumulative_arc_length_.size()) {
    const double seg_length =
      cumulative_arc_length_[closest_seg + 1] - cumulative_arc_length_[closest_seg];
    s_current = cumulative_arc_length_[closest_seg] + closest_t * seg_length;
  } else {
    s_current = cumulative_arc_length_[closest_seg];
  }

  return cumulative_arc_length_.back() - s_current;
}

bool PurePursuitNode::findTargetPoint(const geometry_msgs::msg::Pose & control_pose)
{
  const auto & poses = current_path_->poses;
  if (poses.empty()) {
    return false;
  }
  if (poses.size() < 2) {
    target_point_ = poses.back().pose.position;
    target_idx_ = poses.size() - 1;
    return true;
  }

  const auto & cur = control_pose.position;
  const auto & goal = poses.back().pose.position;
  const double effective_lookahead = getEffectiveLookaheadDistance();

  const double dist_to_goal = std::hypot(cur.x - goal.x, cur.y - goal.y);
  if (dist_to_goal <= effective_lookahead) {
    target_point_ = goal;
    target_idx_ = static_cast<double>(poses.size() - 1);
    return true;
  }

  double min_proj_dist = std::numeric_limits<double>::max();
  size_t proj_seg_idx = closest_idx_;
  for (size_t i = closest_idx_; i + 1 < poses.size(); ++i) {
    const double dist = calc_dinstanse(i, cur);
    if (dist < min_proj_dist) {
      min_proj_dist = dist;
      proj_seg_idx = i;
    }
  }
  closest_idx_ = proj_seg_idx;

  const auto & p1 = poses[closest_idx_].pose.position;
  const auto & p2 = poses[closest_idx_ + 1].pose.position;
  const double dx = p2.x - p1.x;
  const double dy = p2.y - p1.y;
  const double seg_len = std::hypot(dx, dy);

  double t = 0.0;
  if (seg_len > 1e-9) {
    t = ((cur.x - p1.x) * dx + (cur.y - p1.y) * dy) / (seg_len * seg_len);
    t = std::clamp(t, 0.0, 1.0);
  }

  double need = effective_lookahead;
  const double remaining_on_seg = seg_len * (1.0 - t);

  if (remaining_on_seg >= need) {
    const double t_la = t + need / seg_len;
    target_point_.x = p1.x + t_la * dx;
    target_point_.y = p1.y + t_la * dy;
    target_point_.z = p1.z + t_la * (p2.z - p1.z);
    target_idx_ = static_cast<double>(closest_idx_);
    return true;
  }

  need -= remaining_on_seg;
  for (size_t i = closest_idx_ + 1; i + 1 < poses.size(); ++i) {
    const auto & pa = poses[i].pose.position;
    const auto & pb = poses[i + 1].pose.position;
    const double sdx = pb.x - pa.x;
    const double sdy = pb.y - pa.y;
    const double seg = std::hypot(sdx, sdy);
    if (seg < 1e-9) {
      continue;
    }
    if (seg >= need) {
      const double ratio = need / seg;
      target_point_.x = pa.x + ratio * sdx;
      target_point_.y = pa.y + ratio * sdy;
      target_point_.z = pa.z + ratio * (pb.z - pa.z);
      target_idx_ = static_cast<double>(i);
      return true;
    }
    need -= seg;
  }

  target_point_ = goal;
  target_idx_ = static_cast<double>(poses.size() - 1);
  return true;
}

void PurePursuitNode::detect_velocity(const geometry_msgs::msg::Pose & control_pose)
{
  const double yaw = tf2::getYaw(control_pose.orientation);
  const double dx = target_point_.x - control_pose.position.x;
  const double dy = target_point_.y - control_pose.position.y;
  if (!std::isfinite(yaw) || !std::isfinite(dx) || !std::isfinite(dy)) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
      "Invalid pose or target for velocity computation, publishing stop");
    publishStopCmd();
    return;
  }

  const double remaining_path_distance = getRemainingPathDistance(control_pose);
  double linear_x = std::min(effective_max_speed_, max_velocity_);

  if (remaining_path_distance <= goal_tolerance_) {
    linear_x = 0.0;
  } else if (remaining_path_distance <= decel_start_distance_ && deceleration_ > 1e-6) {
    const double decel_distance = remaining_path_distance - goal_tolerance_;
    if (decel_distance > 0.0) {
      linear_x = std::min(linear_x, computeDecelProfileSpeed(decel_distance));
    } else {
      linear_x = 0.0;
    }
  }

  if (remaining_path_distance > goal_tolerance_) {
    const double dist_to_lookahead = std::hypot(dx, dy);
    const double actual_lookahead = std::max(dist_to_lookahead, min_lookahead_distance_);

    if (actual_lookahead > goal_tolerance_) {
      // 車体座標系の横方向偏差から曲率を求める (ia-amr-ros pure_pursuit)
      const double local_y = -std::sin(yaw) * dx + std::cos(yaw) * dy;
      const double curvature = 2.0 * local_y / (actual_lookahead * actual_lookahead);

      if (acceleration_ > 1e-6) {
        constexpr double kMinCurvature = 1e-6;
        const double abs_curvature = std::max(std::abs(curvature), kMinCurvature);
        const double v_max_curvature = std::sqrt(acceleration_ / abs_curvature) *
          curvature_safety_factor_;
        linear_x = std::min(linear_x, v_max_curvature);
      }

      const double angular_z = std::clamp(
        linear_x * curvature, -max_angular_velocity_, max_angular_velocity_);
      pre_speed_ = linear_x;
      last_omega_ = angular_z;
      publishCmdVel(linear_x, angular_z);
      return;
    }
  }

  pre_speed_ = 0.0;
  last_omega_ = 0.0;
  publishStopCmd();
}

double PurePursuitNode::getPathStartYaw()
{
  if (!current_path_ || current_path_->poses.empty()) {
    return 0.0;
  }
  const auto & poses = current_path_->poses;
  constexpr double kMinDist = 0.05;  // [m]

  // 1. ロボット位置から十分離れた最初の経路点への方向
  for (const auto & pose_stamped : poses) {
    const double dx = pose_stamped.pose.position.x - current_pose_.position.x;
    const double dy = pose_stamped.pose.position.y - current_pose_.position.y;
    if (std::hypot(dx, dy) >= kMinDist) {
      return std::atan2(dy, dx);
    }
  }

  // 2. 経路上の最初の有効セグメント方向（スムージング後の短い始端をスキップ）
  for (size_t i = 0; i + 1 < poses.size(); ++i) {
    const double dx = poses[i + 1].pose.position.x - poses[i].pose.position.x;
    const double dy = poses[i + 1].pose.position.y - poses[i].pose.position.y;
    if (std::hypot(dx, dy) >= kMinDist) {
      return std::atan2(dy, dx);
    }
  }

  // 3. 最終点への方向
  const auto & goal = poses.back().pose.position;
  return std::atan2(
    goal.y - current_pose_.position.y,
    goal.x - current_pose_.position.x);
}

double PurePursuitNode::getGoalYaw()
{
  if (!current_path_ || current_path_->poses.empty()) {
    return tf2::getYaw(current_pose_.orientation);
  }

  const auto & poses = current_path_->poses;
  const double goal_yaw = tf2::getYaw(poses.back().pose.orientation);
  if (std::isfinite(goal_yaw)) {
    return goal_yaw;
  }

  for (int i = static_cast<int>(poses.size()) - 2; i >= 0; --i) {
    const double dx = poses[i + 1].pose.position.x - poses[i].pose.position.x;
    const double dy = poses[i + 1].pose.position.y - poses[i].pose.position.y;
    if (std::hypot(dx, dy) > 1e-6) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
        "Goal orientation invalid, using path tangent as goal yaw");
      return std::atan2(dy, dx);
    }
  }

  return tf2::getYaw(current_pose_.orientation);
}

bool PurePursuitNode::isStartAngleReached()
{
  if (!current_path_ || current_path_->poses.empty()) {
    return false;
  }
  double target_yaw = getPathStartYaw();
  double current_yaw = tf2::getYaw(current_pose_.orientation);
  double angle_diff = angles::shortest_angular_distance(current_yaw, target_yaw);
  return std::abs(angle_diff) < rotation_threshold_;
}

bool PurePursuitNode::isGoalReached(
  const geometry_msgs::msg::Pose & current_pose,
  const geometry_msgs::msg::PoseStamped & goal_pose)
{
  const double goal_dist = calculateDistance(current_pose.position, goal_pose.pose.position);
  const double remaining = getRemainingPathDistance(current_pose);
  // 直線距離だけだと Pure Pursuit の横偏差で GOAL_ROTATE に移行できないため、
  // 経路残距離も併用する (ia-amr-ros isNearGoalXY と同じ)
  return remaining <= goal_tolerance_ || goal_dist <= goal_tolerance_;
}

bool PurePursuitNode::isGoalAngleReached()
{
  if (!current_path_ || current_path_->poses.empty()) {
    return false;
  }
  const double goal_yaw = getGoalYaw();
  const double current_yaw = tf2::getYaw(current_pose_.orientation);
  if (!std::isfinite(goal_yaw) || !std::isfinite(current_yaw)) {
    return false;
  }
  const double angle_diff = angles::shortest_angular_distance(current_yaw, goal_yaw);
  return std::abs(angle_diff) < rotation_threshold_;
}

double PurePursuitNode::calculateDistance(
  const geometry_msgs::msg::Point & p1,
  const geometry_msgs::msg::Point & p2)
{
  double dx = p1.x - p2.x;
  double dy = p1.y - p2.y;
  return std::sqrt(dx * dx + dy * dy);
}

bool PurePursuitNode::colisionCheck()
{
  if (!costmap_received_) {
    return false;
  }

  double resolution = costmap_.info.resolution;
  double origin_x = costmap_.info.origin.position.x;
  double origin_y = costmap_.info.origin.position.y;
  int width = costmap_.info.width;
  int height = costmap_.info.height;

  int robot_x_idx = static_cast<int>((current_pose_.position.x - origin_x) / resolution);
  int robot_y_idx = static_cast<int>((current_pose_.position.y - origin_y) / resolution);

  // 1.0mの範囲をチェック
  int radius_cells = static_cast<int>(1.0 / resolution);
  for (int dy = -radius_cells; dy <= radius_cells; ++dy) {
    for (int dx = -radius_cells; dx <= radius_cells; ++dx) {
      int x_idx = robot_x_idx + dx;
      int y_idx = robot_y_idx + dy;
      if (x_idx >= 0 && x_idx < width && y_idx >= 0 && y_idx < height) {
        int index = y_idx * width + x_idx;
        int cost_value = costmap_.data[index];
        const int collision_threshold = 50;
        if (cost_value >= collision_threshold) {
          return true; // 衝突あり
        }
      }
    }
  }
  return false; // 衝突なし
}

void PurePursuitNode::publishMarkers(const geometry_msgs::msg::Point & target_point)
{
  visualization_msgs::msg::MarkerArray marker_array;
  visualization_msgs::msg::Marker target_marker;
  target_marker.header.frame_id = map_frame_id_;
  target_marker.header.stamp = now();
  target_marker.ns = "target_point";
  target_marker.id = 0;
  target_marker.type = visualization_msgs::msg::Marker::SPHERE;
  target_marker.action = visualization_msgs::msg::Marker::ADD;
  target_marker.pose.position = target_point;
  target_marker.pose.orientation.w = 1.0;
  target_marker.scale.x = 0.2;
  target_marker.scale.y = 0.2;
  target_marker.scale.z = 0.2;
  target_marker.color.r = 1.0;
  target_marker.color.g = 0.0;
  target_marker.color.b = 0.0;
  target_marker.color.a = 1.0;
  target_marker.lifetime = rclcpp::Duration(100ms);
  marker_array.markers.push_back(target_marker);
  marker_publisher_->publish(marker_array);
}
} // namespace pure_pursuit_planner

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(pure_pursuit_planner::PurePursuitNode)


