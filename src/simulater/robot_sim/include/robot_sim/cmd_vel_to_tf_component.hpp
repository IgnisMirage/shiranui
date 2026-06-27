#ifndef ROBOT_SIM__CMD_VEL_TO_TF_COMPONENT_HPP_
#define ROBOT_SIM__CMD_VEL_TO_TF_COMPONENT_HPP_

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <tf2_ros/transform_broadcaster.h>

#include <memory>
#include <mutex>
#include <string>

namespace robot_sim
{
// cmd_vel(geometry_msgs/Twist) を受け取り、速度を積分して
// map->base_link の TF を発行する簡易シミュレータノード。
class CmdVelToTfNode : public rclcpp::Node
{
public:
  explicit CmdVelToTfNode(const rclcpp::NodeOptions & node_options = rclcpp::NodeOptions());

private:
  void onCmdVelReceived(const geometry_msgs::msg::Twist::SharedPtr msg);
  void onInitialPoseReceived(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg);
  void updateLoop();

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_subscriber_;
  rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr
    initial_pose_subscriber_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_publisher_;
  rclcpp::TimerBase::SharedPtr update_timer_;

  std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;

  // パラメータ
  std::string map_frame_id_;     // 親フレーム（map）
  std::string base_frame_id_;    // 子フレーム（base_link）
  double publish_rate_;          // TF 発行レート [Hz]

  // ロボットの現在状態（2D）。timer と initialpose コールバックから更新されるため mutex で保護
  double x_;
  double y_;
  double yaw_;
  std::mutex pose_mutex_;

  // 最後に受け取った速度指令
  geometry_msgs::msg::Twist current_cmd_vel_;
  rclcpp::Time last_update_time_;
  std::mutex cmd_vel_mutex_;
};
}  // namespace robot_sim

#endif  // ROBOT_SIM__CMD_VEL_TO_TF_COMPONENT_HPP_
