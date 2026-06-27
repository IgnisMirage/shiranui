#include "stanley_planner/stanley_planner_component.hpp"
#include <angles/angles.h>

using namespace std::chrono_literals;
using std::placeholders::_1;

namespace stanley_planner
{
StanleyNode::StanleyNode(const rclcpp::NodeOptions & node_options)
: Node("stanley_planner", node_options),
  tf_buffer_(std::make_shared<rclcpp::Clock>(RCL_ROS_TIME)),
  tf_listener_(tf_buffer_)
{
  k_gain_ = declare_parameter<double>("k_gain", 1.0);
  k_soft_ = declare_parameter<double>("k_soft", 0.5);
  k_yaw_ = declare_parameter<double>("k_yaw", 1.0);
  front_axle_distance_ = declare_parameter<double>("front_axle_distance", 0.2);
  linear_velocity_ = declare_parameter<double>("linear_velocity", 0.3);
  max_angular_velocity_ = declare_parameter<double>("max_angular_velocity", 1.0);
  goal_tolerance_ = declare_parameter<double>("goal_tolerance", 0.2);
  robot_frame_id_ = declare_parameter<std::string>("robot_frame_id", "base_link");
  map_frame_id_ = declare_parameter<std::string>("map_frame_id", "map");
  rotation_threshold_ = declare_parameter<double>("rotation_threshold", 0.1);
  rotation_velocity_ = declare_parameter<double>("rotation_velocity", 0.5);

  // 内部状態の初期化
  follow_path_ = false;
  is_goal_reached_ = false;
  current_state_ = RobotState::STOP;

  twist_publisher_ = create_publisher<geometry_msgs::msg::Twist>("/auto_cmd_vel", 10);
  marker_publisher_ = create_publisher<visualization_msgs::msg::MarkerArray>("~/marker", 10);
  mode_publisher_ = create_publisher<std_msgs::msg::String>("~/mode", 10);
  path_subscriber_ = create_subscription<nav_msgs::msg::Path>(
    "path", rclcpp::QoS(10).transient_local().reliable().keep_last(1),
    std::bind(&StanleyNode::onPathReceived, this, _1));
  control_timer_ = create_wall_timer(
    10ms, std::bind(&StanleyNode::ControlLoop, this));
}

StanleyNode::~StanleyNode()
{
  if (twist_publisher_) {
    geometry_msgs::msg::Twist stop_cmd;
    stop_cmd.linear.x = 0.0;
    stop_cmd.angular.z = 0.0;
    twist_publisher_->publish(stop_cmd);
  }
}

void StanleyNode::onPathReceived(const nav_msgs::msg::Path::SharedPtr msg)
{
  current_path_ = msg;
  follow_path_ = true;
  is_goal_reached_ = false;
  closest_idx_ = 0;
  current_state_ = RobotState::STOP; // 新しいパスを受け取ったら状態をリセット
}

void StanleyNode::getCurrentPose()
{
  geometry_msgs::msg::TransformStamped transform;
  try {
    transform = tf_buffer_.lookupTransform(
      map_frame_id_, robot_frame_id_, tf2::TimePointZero);
  } catch (tf2::TransformException & ex) {
    RCLCPP_ERROR(get_logger(), "Could not transform %s to %s: %s",
      map_frame_id_.c_str(), robot_frame_id_.c_str(), ex.what());
    return;
  }
  current_pose_.position.x = transform.transform.translation.x;
  current_pose_.position.y = transform.transform.translation.y;
  current_pose_.position.z = transform.transform.translation.z;
  current_pose_.orientation = transform.transform.rotation;
}

void StanleyNode::updateState()
{
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

std::string StanleyNode::getCurrentMode()
{
  switch (current_state_) {
    case RobotState::STOP:         return "STOP";
    case RobotState::START_ROTATE: return "START_ROTATE";
    case RobotState::FOLLOW:       return "FOLLOW";
    case RobotState::GOAL_ROTATE:  return "GOAL_ROTATE";
    case RobotState::GOAL:         return "GOAL";
    default:                       return "UNKNOWN";
  }
}

void StanleyNode::ControlLoop()
{
  // 1. 自己位置取得
  getCurrentPose();
  // 2. 状態更新
  if (!current_path_ || current_path_->poses.empty()) {
    return;
  }
  updateState();

  geometry_msgs::msg::Twist cmd;
  switch (current_state_) {
    case RobotState::STOP:
    case RobotState::GOAL:
      cmd.linear.x = 0.0;
      cmd.angular.z = 0.0;
      twist_publisher_->publish(cmd);
      break;
    case RobotState::START_ROTATE:
      cmd.linear.x = 0.0;
      cmd.angular.z = calculateAngularVelocity(current_path_->poses.front().pose);
      twist_publisher_->publish(cmd);
      break;
    case RobotState::FOLLOW:
      followPath();
      break;
    case RobotState::GOAL_ROTATE:
      cmd.linear.x = 0.0;
      cmd.angular.z = calculateAngularVelocity(current_path_->poses.back().pose);
      twist_publisher_->publish(cmd);
      break;
  }

  std_msgs::msg::String mode_msg;
  mode_msg.data = getCurrentMode();
  mode_publisher_->publish(mode_msg);
}

// 現在位置（前車軸位置）に最も近いパスセグメントの始点インデックスを返す
size_t StanleyNode::findClosestSegment(const geometry_msgs::msg::Point & query)
{
  double min_dist = std::numeric_limits<double>::max();
  size_t best_idx = closest_idx_;
  for (size_t i = closest_idx_; i + 1 < current_path_->poses.size(); ++i) {
    const auto & p1 = current_path_->poses[i].pose.position;
    const auto & p2 = current_path_->poses[i + 1].pose.position;
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double seg_len2 = dx * dx + dy * dy;
    double t = 0.0;
    if (seg_len2 > 1e-9) {
      t = ((query.x - p1.x) * dx + (query.y - p1.y) * dy) / seg_len2;
      t = std::clamp(t, 0.0, 1.0);
    }
    double proj_x = p1.x + t * dx;
    double proj_y = p1.y + t * dy;
    double dist = std::hypot(query.x - proj_x, query.y - proj_y);
    if (dist < min_dist) {
      min_dist = dist;
      best_idx = i;
    }
  }
  return best_idx;
}

// Stanley制御則によるステアリング角を計算する
double StanleyNode::computeStanleySteer(
  const geometry_msgs::msg::Point & front_axle,
  double vehicle_yaw,
  double speed)
{
  size_t seg_idx = findClosestSegment(front_axle);
  closest_idx_ = seg_idx;

  const auto & p1 = current_path_->poses[seg_idx].pose.position;
  const auto & p2 = current_path_->poses[seg_idx + 1].pose.position;

  // パスセグメントの進行方向
  double path_yaw = std::atan2(p2.y - p1.y, p2.x - p1.x);

  // 前車軸位置をセグメントに投影し、横方向誤差（符号付き）を求める
  double dx = p2.x - p1.x;
  double dy = p2.y - p1.y;
  double seg_len2 = dx * dx + dy * dy;
  double t = 0.0;
  if (seg_len2 > 1e-9) {
    t = ((front_axle.x - p1.x) * dx + (front_axle.y - p1.y) * dy) / seg_len2;
    t = std::clamp(t, 0.0, 1.0);
  }
  geometry_msgs::msg::Point proj;
  proj.x = p1.x + t * dx;
  proj.y = p1.y + t * dy;
  proj.z = p1.z + t * (p2.z - p1.z);
  target_point_ = proj;

  // 符号付き横方向誤差: パス進行方向の左側を正とする
  double err_x = front_axle.x - proj.x;
  double err_y = front_axle.y - proj.y;
  double cross_track_error = std::cos(path_yaw) * err_y - std::sin(path_yaw) * err_x;

  // 方位誤差: パス方向と車体方位の差
  double heading_error = angles::shortest_angular_distance(vehicle_yaw, path_yaw);

  // Stanley制御則
  // δ = heading_error + atan2(-k * e, k_soft + v)
  // 左にズレている(e>0)場合は右に切る(負)よう符号を反転
  double cross_track_steer = std::atan2(-k_gain_ * cross_track_error, k_soft_ + std::abs(speed));

  return heading_error + cross_track_steer;
}

void StanleyNode::followPath()
{
  if (current_path_->poses.size() < 2) {
    return;
  }

  double vehicle_yaw = tf2::getYaw(current_pose_.orientation);
  double speed = linear_velocity_;

  // 前車軸位置（差動二輪では中心から前方にオフセットした仮想点）
  geometry_msgs::msg::Point front_axle;
  front_axle.x = current_pose_.position.x + front_axle_distance_ * std::cos(vehicle_yaw);
  front_axle.y = current_pose_.position.y + front_axle_distance_ * std::sin(vehicle_yaw);
  front_axle.z = current_pose_.position.z;

  double steer = computeStanleySteer(front_axle, vehicle_yaw, speed);

  geometry_msgs::msg::Twist cmd_vel;
  cmd_vel.linear.x = speed;
  // ステアリング角を角速度指令へ変換（差動二輪向けにゲインで調整）
  cmd_vel.angular.z = std::clamp(k_yaw_ * steer, -max_angular_velocity_, max_angular_velocity_);
  twist_publisher_->publish(cmd_vel);

  publishMarkers(target_point_);
}

double StanleyNode::calculateAngularVelocity(const geometry_msgs::msg::Pose & target_pose)
{
  double target_yaw = tf2::getYaw(target_pose.orientation);
  double current_yaw = tf2::getYaw(current_pose_.orientation);
  double angle_diff = angles::shortest_angular_distance(current_yaw, target_yaw);

  const double min_angular_vel = 0.1;
  double factor = std::abs(angle_diff) / M_PI_2;  // 90度を基準に正規化
  double angular_vel = factor * max_angular_velocity_;
  angular_vel = std::clamp(angular_vel, min_angular_vel, max_angular_velocity_);

  return (angle_diff > 0) ? angular_vel : -angular_vel;
}

bool StanleyNode::isStartAngleReached()
{
  double target_yaw = tf2::getYaw(current_path_->poses.front().pose.orientation);
  double current_yaw = tf2::getYaw(current_pose_.orientation);
  double angle_diff = angles::shortest_angular_distance(target_yaw, current_yaw);
  return std::abs(angle_diff) < rotation_threshold_;
}

bool StanleyNode::isGoalAngleReached()
{
  double goal_yaw = tf2::getYaw(current_path_->poses.back().pose.orientation);
  double current_yaw = tf2::getYaw(current_pose_.orientation);
  double angle_diff = angles::shortest_angular_distance(goal_yaw, current_yaw);
  return std::abs(angle_diff) < rotation_threshold_;
}

bool StanleyNode::isGoalReached(
  const geometry_msgs::msg::Pose & current_pose,
  const geometry_msgs::msg::PoseStamped & goal_pose)
{
  double distance = calculateDistance(current_pose.position, goal_pose.pose.position);
  return distance < goal_tolerance_;
}

double StanleyNode::calculateDistance(
  const geometry_msgs::msg::Point & p1,
  const geometry_msgs::msg::Point & p2)
{
  double dx = p1.x - p2.x;
  double dy = p1.y - p2.y;
  return std::sqrt(dx * dx + dy * dy);
}

void StanleyNode::publishMarkers(const geometry_msgs::msg::Point & target_point)
{
  visualization_msgs::msg::MarkerArray marker_array;
  visualization_msgs::msg::Marker target_marker;
  target_marker.header.frame_id = map_frame_id_;
  target_marker.header.stamp = now();
  target_marker.ns = "stanley_target_point";
  target_marker.id = 0;
  target_marker.type = visualization_msgs::msg::Marker::SPHERE;
  target_marker.action = visualization_msgs::msg::Marker::ADD;
  target_marker.pose.position = target_point;
  target_marker.pose.orientation.w = 1.0;
  target_marker.scale.x = 0.2;
  target_marker.scale.y = 0.2;
  target_marker.scale.z = 0.2;
  target_marker.color.r = 0.0;
  target_marker.color.g = 1.0;
  target_marker.color.b = 0.0;
  target_marker.color.a = 1.0;
  target_marker.lifetime = rclcpp::Duration(100ms);
  marker_array.markers.push_back(target_marker);
  marker_publisher_->publish(marker_array);
}
} // namespace stanley_planner

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(stanley_planner::StanleyNode)
