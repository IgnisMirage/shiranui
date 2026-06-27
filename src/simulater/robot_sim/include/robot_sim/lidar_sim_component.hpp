#ifndef ROBOT_SIM__LIDAR_SIM_COMPONENT_HPP_
#define ROBOT_SIM__LIDAR_SIM_COMPONENT_HPP_

#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/static_transform_broadcaster.h>

#include <memory>
#include <random>
#include <string>
#include <vector>

namespace robot_sim
{
class LidarSimNode : public rclcpp::Node
{
public:
  explicit LidarSimNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  void onMapReceived(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);
  void publishStaticLaserTf();
  void publishScan();

  bool mapReceived() const;
  bool lookupLaserPose(double & x, double & y, double & yaw) const;
  bool isInsideMap(double map_x, double map_y) const;
  enum class CellState { Outside, Unknown, Free, Occupied };
  CellState getCellState(double map_x, double map_y) const;
  double castRay(double origin_x, double origin_y, double ray_yaw) const;
  double applyRangeNoise(double true_range);
  float applyIntensityNoise(float base_intensity);
  double sampleGaussian(double stddev);

  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_subscriber_;
  rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr scan_publisher_;
  rclcpp::TimerBase::SharedPtr scan_timer_;
  std::unique_ptr<tf2_ros::StaticTransformBroadcaster> static_tf_broadcaster_;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;

  nav_msgs::msg::OccupancyGrid map_;
  bool map_received_ = false;

  std::string map_frame_id_;
  std::string base_frame_id_;
  std::string laser_frame_id_;
  std::string scan_topic_;
  double publish_rate_;
  double range_min_;
  double range_max_;
  double angle_min_;
  double angle_max_;
  int num_beams_;
  double angular_resolution_deg_;  // >0 のとき num_beams を自動計算
  int occupied_threshold_;
  double laser_x_;
  double laser_y_;
  double laser_yaw_;
  bool publish_laser_tf_;

  bool enable_noise_;
  double range_stddev_;
  double dropout_probability_;
  double intensity_stddev_;
  double range_resolution_;  // 距離分解能 [m]（0=無効）
  int random_seed_;

  std::mt19937 rng_;
  std::uniform_real_distribution<double> uniform_dist_{0.0, 1.0};
  std::normal_distribution<double> normal_dist_{0.0, 1.0};
};

}  // namespace robot_sim

#endif  // ROBOT_SIM__LIDAR_SIM_COMPONENT_HPP_