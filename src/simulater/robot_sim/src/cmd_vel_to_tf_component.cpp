#include "robot_sim/cmd_vel_to_tf_component.hpp"

#include <tf2/LinearMath/Quaternion.h>
#include <tf2/utils.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

#include <cmath>

namespace robot_sim
{
namespace
{
bool isFinite(double v)
{
  return std::isfinite(v);
}

bool isValidTwist(const geometry_msgs::msg::Twist & cmd)
{
  return isFinite(cmd.linear.x) && isFinite(cmd.linear.y) && isFinite(cmd.linear.z) &&
         isFinite(cmd.angular.x) && isFinite(cmd.angular.y) && isFinite(cmd.angular.z);
}

bool isValidPose(double x, double y, double yaw)
{
  return isFinite(x) && isFinite(y) && isFinite(yaw);
}
}  // namespace
CmdVelToTfNode::CmdVelToTfNode(const rclcpp::NodeOptions & node_options)
: Node("cmd_vel_to_tf", node_options),
  x_(0.0),
  y_(0.0),
  yaw_(0.0)
{
  // パラメータの宣言
  map_frame_id_ = this->declare_parameter<std::string>("map_frame_id", "map");
  base_frame_id_ = this->declare_parameter<std::string>("base_frame_id", "base_link");
  publish_rate_ = this->declare_parameter<double>("publish_rate", 50.0);
  x_ = this->declare_parameter<double>("initial_x", 0.0);
  y_ = this->declare_parameter<double>("initial_y", 0.0);
  yaw_ = this->declare_parameter<double>("initial_yaw", 0.0);

  tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

  cmd_vel_subscriber_ = this->create_subscription<geometry_msgs::msg::Twist>(
    "cmd_vel", 10,
    std::bind(&CmdVelToTfNode::onCmdVelReceived, this, std::placeholders::_1));

  // RViz / Foxglove の "2D Pose Estimate" などで初期位置をリセットできるようにする
  initial_pose_subscriber_ =
    this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
      "initialpose", 10,
      std::bind(&CmdVelToTfNode::onInitialPoseReceived, this, std::placeholders::_1));

  odom_publisher_ = this->create_publisher<nav_msgs::msg::Odometry>("odom", 10);
  last_update_time_ = this->now();

  const auto period = std::chrono::duration<double>(1.0 / publish_rate_);
  update_timer_ = this->create_wall_timer(
    std::chrono::duration_cast<std::chrono::nanoseconds>(period),
    std::bind(&CmdVelToTfNode::updateLoop, this));

  RCLCPP_INFO(
    this->get_logger(),
    "robot_sim started: %s -> %s @ %.1f Hz (init: x=%.2f, y=%.2f, yaw=%.2f)",
    map_frame_id_.c_str(), base_frame_id_.c_str(), publish_rate_, x_, y_, yaw_);
}

void CmdVelToTfNode::onCmdVelReceived(const geometry_msgs::msg::Twist::SharedPtr msg)
{
  if (!isValidTwist(*msg)) {
    RCLCPP_WARN_THROTTLE(
      this->get_logger(), *this->get_clock(), 2000,
      "cmd_vel に NaN/Inf が含まれているため無視します");
    return;
  }
  std::lock_guard<std::mutex> lock(cmd_vel_mutex_);
  current_cmd_vel_ = *msg;
}

void CmdVelToTfNode::onInitialPoseReceived(
  const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg)
{
  // initialpose は map フレーム基準を想定。異なる場合は警告のみ（簡易のため変換はしない）
  if (!msg->header.frame_id.empty() && msg->header.frame_id != map_frame_id_) {
    RCLCPP_WARN(
      this->get_logger(),
      "initialpose の frame_id '%s' が map_frame_id '%s' と異なります。"
      "そのまま map 基準として扱います。",
      msg->header.frame_id.c_str(), map_frame_id_.c_str());
  }

  const double yaw = tf2::getYaw(msg->pose.pose.orientation);
  if (!isValidPose(msg->pose.pose.position.x, msg->pose.pose.position.y, yaw)) {
    RCLCPP_WARN(
      this->get_logger(),
      "initialpose に NaN/Inf が含まれているため無視します");
    return;
  }

  {
    std::lock_guard<std::mutex> lock(pose_mutex_);
    x_ = msg->pose.pose.position.x;
    y_ = msg->pose.pose.position.y;
    yaw_ = yaw;
  }

  RCLCPP_INFO(
    this->get_logger(),
    "initialpose を受信: x=%.2f, y=%.2f, yaw=%.2f に位置をリセットしました。",
    msg->pose.pose.position.x, msg->pose.pose.position.y, yaw);
}

void CmdVelToTfNode::updateLoop()
{
  const rclcpp::Time now = this->now();
  double dt = (now - last_update_time_).seconds();
  last_update_time_ = now;

  if (dt <= 0.0 || dt > 1.0) {
    return;
  }

  geometry_msgs::msg::Twist cmd;
  {
    std::lock_guard<std::mutex> lock(cmd_vel_mutex_);
    cmd = current_cmd_vel_;
  }

  const double vx = cmd.linear.x;
  const double vy = cmd.linear.y;
  const double wz = cmd.angular.z;

  if (!isValidTwist(cmd)) {
    RCLCPP_WARN_THROTTLE(
      this->get_logger(), *this->get_clock(), 2000,
      "cmd_vel に NaN/Inf が含まれているため積分をスキップします");
    return;
  }

  double x, y, yaw;
  {
    std::lock_guard<std::mutex> lock(pose_mutex_);
    if (!isValidPose(x_, y_, yaw_)) {
      RCLCPP_ERROR_THROTTLE(
        this->get_logger(), *this->get_clock(), 2000,
        "内部姿勢が NaN/Inf です。Foxglove の 2D Pose Estimate で初期位置を再設定してください");
      return;
    }

    x_ += (vx * std::cos(yaw_) - vy * std::sin(yaw_)) * dt;
    y_ += (vx * std::sin(yaw_) + vy * std::cos(yaw_)) * dt;
    yaw_ += wz * dt;

    yaw_ = std::atan2(std::sin(yaw_), std::cos(yaw_));
    if (!isValidPose(x_, y_, yaw_)) {
      RCLCPP_ERROR(
        this->get_logger(),
        "姿勢積分後に NaN/Inf が発生しました。2D Pose Estimate で初期位置を再設定してください");
      return;
    }
    x = x_;
    y = y_;
    yaw = yaw_;
  }

  tf2::Quaternion q;
  q.setRPY(0.0, 0.0, yaw);

  geometry_msgs::msg::TransformStamped tf;
  tf.header.stamp = now;
  tf.header.frame_id = map_frame_id_;
  tf.child_frame_id = base_frame_id_;
  tf.transform.translation.x = x;
  tf.transform.translation.y = y;
  tf.transform.translation.z = 0.0;
  tf.transform.rotation = tf2::toMsg(q);
  tf_broadcaster_->sendTransform(tf);

  // 参考用に odom も発行
  nav_msgs::msg::Odometry odom;
  odom.header.stamp = now;
  odom.header.frame_id = map_frame_id_;
  odom.child_frame_id = base_frame_id_;
  odom.pose.pose.position.x = x;
  odom.pose.pose.position.y = y;
  odom.pose.pose.position.z = 0.0;
  odom.pose.pose.orientation = tf2::toMsg(q);
  odom.twist.twist = cmd;
  odom_publisher_->publish(odom);
}
}  // namespace robot_sim

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(robot_sim::CmdVelToTfNode)
