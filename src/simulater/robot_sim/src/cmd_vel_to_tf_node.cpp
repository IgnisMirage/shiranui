#include "rclcpp/rclcpp.hpp"
#include "robot_sim/cmd_vel_to_tf_component.hpp"
#include <memory>

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::executors::SingleThreadedExecutor exec;
    rclcpp::NodeOptions options;
    auto component = std::make_shared<robot_sim::CmdVelToTfNode>(options);
    exec.add_node(component);
    exec.spin();
    rclcpp::shutdown();
    return 0;
}
