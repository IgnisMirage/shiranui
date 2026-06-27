#ifndef PURE_PURSUIT_PLANNER__PURE_PURSUIT_PLANNER_COMPONENT_HPP_
#define PURE_PURSUIT_PLANNER__PURE_PURSUIT_PLANNER_COMPONENT_HPP_

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
#include <nav_msgs/msg/occupancy_grid.hpp>

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

namespace pure_pursuit_planner
{
class PurePursuitNode : public rclcpp::Node
{
public:
  explicit PurePursuitNode(const rclcpp::NodeOptions & node_options = rclcpp::NodeOptions());
  ~PurePursuitNode();

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
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr costmap_sub_;
  rclcpp::TimerBase::SharedPtr control_timer_;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;

  // パラメータ
  double lookahead_distance_;       // 注視点距離 [m]
  double min_lookahead_distance_;   // 曲率計算用の注視点距離下限 [m]
  double linear_velocity_;          // 巡航速度 [m/s]
  double max_angular_velocity_;     // 最大角速度 [rad/s]
  double goal_tolerance_;           // ゴール許容誤差 [m]
  std::string robot_frame_id_;
  std::string map_frame_id_;
  double rotation_threshold_;
  double rotation_velocity_;
  double max_velocity_;
  double deceleration_;
  double acceleration_;             // 曲率ベース速度制限用 [m/s^2]
  double curvature_safety_factor_;  // 曲率速度制限の安全係数
  double sensor_delay_;             // 制御用姿勢予測の遅延補償 [s]
  double goal_approach_distance_;   // ゴール手前の低速区間 [m]
  double goal_approach_speed_;      // ゴール手前の低速 [m/s]
  double slow_speed_lookahead_threshold_;  // この速度以下で短注視点 [m/s]
  double slow_speed_lookahead_distance_; // 低速時の固定注視点距離 [m]
  double target_idx_;
  size_t closest_idx_ = 0;

  double pre_speed_ = 0.0;
  double last_omega_ = 0.0;
  double effective_max_speed_ = 0.0;
  double decel_start_distance_ = 0.0;
  double total_path_length_ = 0.0;
  std::vector<double> cumulative_arc_length_;

  // 内部状態変数
  nav_msgs::msg::Path::SharedPtr current_path_; // 現在のパス
  bool follow_path_;                            // パスを受け取ったかどうか
  bool is_goal_reached_;                        // ゴールに到達したかどうか
  RobotState current_state_;                    // 現在のロボット状態
  geometry_msgs::msg::Point target_point_;      // 現在のターゲットポイント
  
  bool costmap_received_ = false;
  nav_msgs::msg::OccupancyGrid costmap_;

  
  // コールバック関数
  void onPathReceived(const nav_msgs::msg::Path::SharedPtr msg);
  void CostmapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);
  void ControlLoop();
  // Pure Pursuit アルゴリズムの実装関数
  bool findTargetPoint(const geometry_msgs::msg::Pose & control_pose);
  void detect_velocity(const geometry_msgs::msg::Pose & control_pose);
  double calc_dinstanse(size_t index, const geometry_msgs::msg::Point & query) const;
  double getRemainingPathDistance(const geometry_msgs::msg::Pose & pose) const;
  double getEffectiveLookaheadDistance() const;
  void computeArcLength();
  void updatePathSpeedProfile();
  double computeDecelProfileSpeed(double decel_distance) const;
  bool isGoalApproachEnabled() const;
  geometry_msgs::msg::Pose computeControlPose() const;
  double calculateAngularVelocity(const geometry_msgs::msg::Pose & target_point);

  void updateState();
  bool getCurrentPose();
  geometry_msgs::msg::Pose current_pose_;
  bool pose_valid_ = false;
  void publishMarkers(const geometry_msgs::msg::Point & target_point);

  bool isStartAngleReached();
  bool isGoalAngleReached();
  // 経路の最初のセグメント方向（pose[0]->pose[1]）の yaw
  double getPathStartYaw();
  double getGoalYaw();
  void publishCmdVel(double linear_x, double angular_z);
  void publishStopCmd();
  bool isValidPath(const nav_msgs::msg::Path & path) const;


  std::string getCurrentMode();

  bool isGoalReached(
    const geometry_msgs::msg::Pose & current_pose,
    const geometry_msgs::msg::PoseStamped & goal_pose);

  double calculateDistance(
    const geometry_msgs::msg::Point & p1,
    const geometry_msgs::msg::Point & p2);

  bool colisionCheck();
  bool isOnDetour_ = false;
    
};
} // namespace pure_pursuit_planner

#endif  // PURE_PURSUIT_PLANNER__PURE_PURSUIT_PLANNER_COMPONENT_HPP_

