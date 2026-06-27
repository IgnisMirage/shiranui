#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "mid360/mid360_driver.hpp"

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<mid360::Mid360Driver>();
  if (!node->start()) {
    RCLCPP_ERROR(node->get_logger(), "failed to start driver");
    node->stop();
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::spin(node);
  node->stop();
  rclcpp::shutdown();
  return 0;
}
