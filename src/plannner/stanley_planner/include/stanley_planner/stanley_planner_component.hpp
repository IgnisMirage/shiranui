#ifndef STANLEY_PLANNER__STANLEY_PLANNER_COMPONENT_HPP_
#define STANLEY_PLANNER__STANLEY_PLANNER_COMPONENT_HPP_

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <std_msgs/msg/string.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <nav_msgs/msg/path.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <tf2/utils.h>
#include <visualization_msgs/msg/marker.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

namespace stanley_planner
{
class StanleyNode : public rclcpp::Node
{
public:
  explicit StanleyNode(const rclcpp::NodeOptions & node_options = rclcpp::NodeOptions());
  ~StanleyNode();

private:
  // ロボットの状態を表す列挙型
  enum class RobotState {
    STOP,           // 停止状態
    START_ROTATE,   // 開始回転状態
    FOLLOW,         // 通常のパス追従状態
    GOAL_ROTATE,    // ゴールに向かって回転する状態
    GOAL            // ゴール到達状態
  };

  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr twist_publisher_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr marker_publisher_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr mode_publisher_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr path_subscriber_;
  rclcpp::TimerBase::SharedPtr control_timer_;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;

  // パラメータ
  double k_gain_;                 // 横方向誤差ゲイン（Stanleyのk）
  double k_soft_;                 // 低速時の発散を防ぐソフトニング定数
  double k_yaw_;                  // ステアリング指令から角速度への変換ゲイン
  double front_axle_distance_;    // ロボット中心から前車軸までの距離 [m]
  double linear_velocity_;        // 線形速度 [m/s]
  double max_angular_velocity_;   // 最大角速度 [rad/s]
  double goal_tolerance_;         // ゴール許容誤差 [m]
  std::string robot_frame_id_;    // ロボットのフレームID
  std::string map_frame_id_;      // マップのフレームID
  double rotation_threshold_;     // 回転状態の角度閾値 [rad]
  double rotation_velocity_;      // その場回転時の角速度 [rad/s]

  size_t closest_idx_ = 0;

  // 内部状態変数
  nav_msgs::msg::Path::SharedPtr current_path_; // 現在のパス
  bool follow_path_;                            // パスを受け取ったかどうか
  bool is_goal_reached_;                        // ゴールに到達したかどうか
  RobotState current_state_;                    // 現在のロボット状態
  geometry_msgs::msg::Pose current_pose_;       // 現在の自己位置
  geometry_msgs::msg::Point target_point_;      // 可視化用の最近傍投影点

  // コールバック関数
  void onPathReceived(const nav_msgs::msg::Path::SharedPtr msg);
  void ControlLoop();

  // Stanley アルゴリズムの実装
  void followPath();
  size_t findClosestSegment(const geometry_msgs::msg::Point & query);
  double computeStanleySteer(
    const geometry_msgs::msg::Point & front_axle,
    double vehicle_yaw,
    double speed);

  // 状態マシン関連の関数
  void updateState();
  void getCurrentPose();
  double calculateAngularVelocity(const geometry_msgs::msg::Pose & target_pose);
  bool isStartAngleReached();
  bool isGoalAngleReached();
  bool isGoalReached(
    const geometry_msgs::msg::Pose & current_pose,
    const geometry_msgs::msg::PoseStamped & goal_pose);
  std::string getCurrentMode();
  void publishMarkers(const geometry_msgs::msg::Point & target_point);

  // ユーティリティ関数
  double calculateDistance(
    const geometry_msgs::msg::Point & p1,
    const geometry_msgs::msg::Point & p2);
};
} // namespace stanley_planner

#endif  // STANLEY_PLANNER__STANLEY_PLANNER_COMPONENT_HPP_
