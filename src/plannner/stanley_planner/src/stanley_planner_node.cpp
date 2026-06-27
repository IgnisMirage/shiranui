#include "rclcpp/rclcpp.hpp"
#include "stanley_planner/stanley_planner_component.hpp"
#include <memory>

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::executors::SingleThreadedExecutor exec;
    rclcpp::NodeOptions options;
    auto component = std::make_shared<stanley_planner::StanleyNode>(options);
    exec.add_node(component);
    exec.spin();
    rclcpp::shutdown();
    return 0;
}
