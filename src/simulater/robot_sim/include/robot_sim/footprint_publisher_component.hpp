#ifndef ROBOT_SIM__FOOTPRINT_PUBLISHER_COMPONENT_HPP_
#define ROBOT_SIM__FOOTPRINT_PUBLISHER_COMPONENT_HPP_

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/polygon_stamped.hpp>

#include <string>
#include <vector>

namespace robot_sim
{
class FootprintPublisherNode : public rclcpp::Node
{
public:
  explicit FootprintPublisherNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  bool loadFootprintFromParams();
  void publishFootprint();

  rclcpp::Publisher<geometry_msgs::msg::PolygonStamped>::SharedPtr footprint_publisher_;
  rclcpp::TimerBase::SharedPtr publish_timer_;

  geometry_msgs::msg::PolygonStamped footprint_msg_;
  std::string frame_id_;
  double publish_rate_;
  std::vector<std::string> vertices_;
};

}  // namespace robot_sim

#endif  // ROBOT_SIM__FOOTPRINT_PUBLISHER_COMPONENT_HPP_
