#include "rclcpp/rclcpp.hpp"
#include "tg15_driver/tg15_driver.hpp"
#include <memory>

int main(int argc, char ** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::NodeOptions options;
    auto node = std::make_shared<tg15_driver::TG15Driver>(options);
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
