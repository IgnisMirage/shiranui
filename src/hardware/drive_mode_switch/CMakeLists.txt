cmake_minimum_required(VERSION 3.8)
project(drive_mode_switch)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(include)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)

add_executable(drive_mode_switch_node src/drive_mode_switch.cpp)
ament_target_dependencies(drive_mode_switch_node rclcpp geometry_msgs sensor_msgs std_msgs)

install(TARGETS
  drive_mode_switch_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
