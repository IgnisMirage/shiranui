#ifndef PURE_PURSUIT_PLANNER__PURE_PURSUIT_PLANNER_COMPONENT_HPP_
#define PURE_PURSUIT_PLANNER__PURE_PURSUIT_PLANNER_COMPONENT_HPP_

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <nav_msgs/msg/path.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/empty.hpp>
#include <std_msgs/msg/float32.hpp>
#include <std_msgs/msg/string.hpp>
#include <tf2/utils.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <visualization_msgs/msg/marker.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include <limits>
#include <string>
#include <vector>

namespace pure_pursuit_planner
{
// ia-amr-ros の pure_pursuit (PurePursuit + PathFollowerManager) を ROS 2 に移植したもの。
class PurePursuitNode : public rclcpp::Node
{
public:
  explicit PurePursuitNode(const rclcpp::NodeOptions & node_options = rclcpp::NodeOptions());
  ~PurePursuitNode();

private:
  enum class State {
    WAIT_PATH,               // 経路未受信
    ROTATE_AT_START,         // スタート時の位置合わせ
    FOLLOW_PATH,             // 経路追従
    ROTATE_AT_GOAL,          // ゴールXY到達後の角度合わせ
    ARRIVED_GOAL,            // ゴール位置・姿勢一致
    PATH_DEVIATION_WARNING,  // 経路逸脱（回復閾値以内で自動復帰）
  };

  // --- ROS インターフェース ---
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr path_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr speed_sub_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr brake_sub_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr slowdown_sub_;
  rclcpp::Subscription<std_msgs::msg::Empty>::SharedPtr pause_sub_;
  rclcpp::Subscription<std_msgs::msg::Empty>::SharedPtr resume_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr mode_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr following_pub_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr reached_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr marker_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr approach_zone_pub_;
  rclcpp::TimerBase::SharedPtr control_timer_;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;

  // --- パラメータ ---
  std::string robot_frame_id_;
  std::string map_frame_id_;
  double control_rate_;
  double transition_wait_time_;
  double goal_standstill_linear_thresh_;
  double goal_standstill_angular_thresh_;
  double goal_standstill_confirm_time_;
  double goal_standstill_timeout_;
  double odom_timeout_;
  double start_tolerance_ang_;
  double goal_tolerance_dist_;
  double goal_rotation_only_distance_threshold_;
  double goal_tolerance_ang_;
  double turn_in_place_threshold_;
  double align_angular_vel_;
  double align_angular_acceleration_;
  double align_slowdown_angle_;
  double align_slowdown_angular_vel_;
  double align_angular_deceleration_;
  double acceleration_;
  double deceleration_;
  double max_angular_acceleration_;
  double max_angular_deceleration_;
  double slowdown_deceleration_;
  double brake_deceleration_;
  double brake_timeout_;
  double cruise_speed_;
  double min_cmd_linear_abs_;
  double min_cmd_angular_abs_;
  double max_angular_velocity_;
  double start_approach_distance_;
  double start_approach_speed_;
  double goal_approach_distance_;
  double goal_approach_speed_;
  double post_stop_recovery_distance_;
  double post_stop_recovery_speed_;
  double curvature_safety_factor_;
  double lookahead_distance_;
  double min_lookahead_distance_;
  double slow_speed_lookahead_threshold_;
  double slow_speed_lookahead_distance_;
  double path_deviation_threshold_;
  double path_deviation_recovery_threshold_;
  bool enable_path_deviation_check_;
  double sensor_delay_;
  bool publish_approach_zone_marker_;

  // --- 状態 ---
  State state_ = State::WAIT_PATH;
  nav_msgs::msg::Path current_path_;
  std::vector<double> cumulative_arc_length_;
  geometry_msgs::msg::Pose current_pose_;
  bool pose_received_ = false;

  bool rotate_at_start_complete_ = false;
  bool rotate_at_goal_complete_ = false;
  rclcpp::Time rotate_at_start_ready_time_{0, 0, RCL_ROS_TIME};
  rclcpp::Time rotate_at_goal_ready_time_{0, 0, RCL_ROS_TIME};

  // ゴール姿勢合わせ前の静止確認 (odom 実速度)
  double measured_linear_speed_ = 0.0;
  double measured_angular_speed_ = 0.0;
  rclcpp::Time last_odom_time_{0, 0, RCL_ROS_TIME};
  rclcpp::Time goal_standstill_since_{0, 0, RCL_ROS_TIME};
  bool rotate_at_goal_standstill_confirmed_ = false;

  // 外部入力
  bool is_brake_active_ = false;
  bool is_slowdown_ = false;
  bool is_paused_ = false;
  double external_speed_limit_ = std::numeric_limits<double>::max();
  rclcpp::Time last_brake_msg_time_{0, 0, RCL_ROS_TIME};

  // 停止後リカバリ
  double post_stop_recovery_start_dist_ = -1.0;

  // 旋回方向固定
  int align_direction_sign_ = 0;
  bool align_force_goal_flag_ = false;

  // 直前の出力
  double last_cmd_linear_x_ = 0.0;
  double last_cmd_angular_z_ = 0.0;
  rclcpp::Time last_cmd_time_{0, 0, RCL_ROS_TIME};

  // --- PurePursuit 本体の状態 ---
  double effective_max_speed_ = 0.0;
  double decel_start_distance_ = 0.0;
  double total_path_length_ = 0.0;
  size_t last_closest_index_ = 0;
  double pre_speed_ = 0.0;
  double last_omega_ = 0.0;
  double pp_lookahead_distance_ = 0.0;  // 現在の注視点距離（低速時に切り替わる）
  double priority_speed_ = std::numeric_limits<double>::max();

  // --- コールバック ---
  void pathCallback(const nav_msgs::msg::Path::SharedPtr msg);
  void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg);
  void speedCallback(const std_msgs::msg::Float32::SharedPtr msg);
  void brakeCallback(const std_msgs::msg::Bool::SharedPtr msg);
  void slowdownCallback(const std_msgs::msg::Bool::SharedPtr msg);
  void pauseCallback(const std_msgs::msg::Empty::SharedPtr msg);
  void resumeCallback(const std_msgs::msg::Empty::SharedPtr msg);

  // --- 制御ループ ---
  void controlLoop();
  bool getCurrentPoseFromTF();
  void updateStateOnPath();
  void updateStateOnPose();
  void publishStatusTopics();
  static std::string stateToString(State s);

  // --- 判定 ---
  bool isAtStandstill() const;
  bool isNearStartYaw();
  bool isNearGoalXY();
  bool isNearGoalYaw();
  bool isValidPath(const nav_msgs::msg::Path & path) const;

  // --- 旋回整合 ---
  geometry_msgs::msg::Twist calcAlignCmd(double target_yaw, double tolerance, double angular_vel);
  geometry_msgs::msg::Twist calcInitAlignCmd();
  geometry_msgs::msg::Twist calcGoalAlignCmd();

  // --- 経路追従 (PurePursuit) ---
  void setPurePursuitPath();
  geometry_msgs::msg::Twist calcPurePursuitCmd();
  geometry_msgs::msg::Pose computePredictedPose() const;
  geometry_msgs::msg::Point getLookaheadPointImpl(const geometry_msgs::msg::Pose & pose);
  geometry_msgs::msg::Point getLookaheadPoint();
  bool isGoalApproachEnabled() const;
  double computeDecelProfileSpeed(double decel_distance) const;
  double getRemainingPathDistanceImpl(const geometry_msgs::msg::Pose & pose) const;
  double getRemainingPathDistance() const;
  double calculatePathDeviationDistance() const;
  void computeArcLength();

  // --- 出力整形 ---
  void startPostStopRecovery();
  bool isPostStopRecoveryActive() const;
  bool shouldEmergencyBrake() const;
  geometry_msgs::msg::Twist applyAccelerationLimits(const geometry_msgs::msg::Twist & cmd);
  double applyAngularAccelLimit(double desired_angular_vel, bool in_place_rotation = false);
  void applyCmdVelMinimumMagnitude(geometry_msgs::msg::Twist & cmd, bool enforce_minimum = true);
  void publishCmd(const geometry_msgs::msg::Twist & cmd);
  double elapsedSince(const rclcpp::Time & t) const;

  // --- 角度 ---
  double getCurrentYaw() const;
  double getGoalYaw() const;
  double getInitialTargetYaw();

  // --- 可視化 ---
  void publishLookaheadMarker(const geometry_msgs::msg::Point & lookahead);
  void publishApproachZoneMarkers();
};
}  // namespace pure_pursuit_planner

#endif  // PURE_PURSUIT_PLANNER__PURE_PURSUIT_PLANNER_COMPONENT_HPP_
