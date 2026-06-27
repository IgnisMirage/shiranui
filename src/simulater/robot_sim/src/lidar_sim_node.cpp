#include "rclcpp/rclcpp.hpp"
#include "robot_sim/lidar_sim_component.hpp"

#include <memory>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::executors::SingleThreadedExecutor exec;
  const rclcpp::NodeOptions options;
  auto component = std::make_shared<robot_sim::LidarSimNode>(options);
  exec.add_node(component);
  exec.spin();
  rclcpp::shutdown();
  return 0;
}
