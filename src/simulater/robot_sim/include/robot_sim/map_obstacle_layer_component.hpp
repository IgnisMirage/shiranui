#ifndef ROBOT_SIM__MAP_OBSTACLE_LAYER_COMPONENT_HPP_
#define ROBOT_SIM__MAP_OBSTACLE_LAYER_COMPONENT_HPP_

#include <nav_msgs/msg/occupancy_grid.hpp>
#include <rclcpp/rclcpp.hpp>

#include <string>
#include <vector>

namespace robot_sim
{

enum class MotionPattern
{
  Linear,
  Circular,
  Loop,
};

struct MapBounds
{
  double min_x = 0.0;
  double max_x = 0.0;
  double min_y = 0.0;
  double max_y = 0.0;
  bool valid = false;
};

struct StaticObstacle
{
  double center_x;
  double center_y;
  double width;
  double height;
};

struct DynamicObstacle
{
  MotionPattern pattern = MotionPattern::Linear;
  double width = 0.0;
  double height = 0.0;
  double speed = 0.0;
  double start_x = 0.0;
  double start_y = 0.0;
  double end_x = 0.0;
  double end_y = 0.0;
  double path_length = 0.0;
  double center_x = 0.0;
  double center_y = 0.0;
  double radius = 0.0;
  double bounds_min_x = 0.0;
  double bounds_min_y = 0.0;
  double bounds_max_x = 0.0;
  double bounds_max_y = 0.0;
  double loop_perimeter = 0.0;
};

class MapObstacleLayerNode : public rclcpp::Node
{
public:
  explicit MapObstacleLayerNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  bool loadStaticObstaclesFromParams();
  bool loadDynamicObstaclesFromParams();
  void onMapReceived(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);
  void onUpdateTimer();
  void publishMergedMap();
  void updateMapBounds(const nav_msgs::msg::OccupancyGrid & map);
  void updateFreeSpaceBounds(const nav_msgs::msg::OccupancyGrid & map);
  void clampObstaclesToMap();
  bool snapCenterToFreeSpace(double & center_x, double & center_y) const;
  void clampCenterToMap(double & center_x, double & center_y, double width, double height) const;
  void paintRect(
    nav_msgs::msg::OccupancyGrid & map,
    double center_x, double center_y, double width, double height) const;
  void getDynamicObstacleCenter(const DynamicObstacle & obstacle, double & center_x, double & center_y) const;

  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_subscriber_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr map_publisher_;
  rclcpp::TimerBase::SharedPtr update_timer_;

  std::vector<StaticObstacle> static_obstacles_;
  std::vector<DynamicObstacle> dynamic_obstacles_;
  std::vector<std::string> static_obstacle_params_;
  std::vector<std::string> dynamic_obstacle_params_;
  nav_msgs::msg::OccupancyGrid base_map_;
  MapBounds map_bounds_;
  MapBounds free_space_bounds_;
  bool has_base_map_ = false;
  bool has_free_space_bounds_ = false;
  rclcpp::Time start_time_;
  std::string input_map_topic_;
  std::string output_map_topic_;
  double update_rate_;
  int8_t obstacle_cost_value_;
};

}  // namespace robot_sim

#endif  // ROBOT_SIM__MAP_OBSTACLE_LAYER_COMPONENT_HPP_
