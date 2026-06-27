cmake_minimum_required(VERSION 3.8)
project(robot_sim)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()


ament_auto_add_library(robot_sim SHARED
  src/cmd_vel_to_tf_component.cpp
  src/lidar_sim_component.cpp
  src/footprint_publisher_component.cpp
  src/map_obstacle_layer_component.cpp
)

rclcpp_components_register_node(robot_sim
  PLUGIN "robot_sim::CmdVelToTfNode"
  EXECUTABLE cmd_vel_to_tf_component
)

rclcpp_components_register_node(robot_sim
  PLUGIN "robot_sim::LidarSimNode"
  EXECUTABLE lidar_sim_component
)

rclcpp_components_register_node(robot_sim
  PLUGIN "robot_sim::FootprintPublisherNode"
  EXECUTABLE footprint_publisher_component
)

rclcpp_components_register_node(robot_sim
  PLUGIN "robot_sim::MapObstacleLayerNode"
  EXECUTABLE map_obstacle_layer_component
)


ament_auto_add_executable(cmd_vel_to_tf_node
  src/cmd_vel_to_tf_node.cpp
)

target_link_libraries(cmd_vel_to_tf_node
  robot_sim
)

ament_auto_add_executable(lidar_sim_node
  src/lidar_sim_node.cpp
)

target_link_libraries(lidar_sim_node
  robot_sim
)

ament_auto_add_executable(footprint_publisher_node
  src/footprint_publisher_node.cpp
)

target_link_libraries(footprint_publisher_node
  robot_sim
)

ament_auto_add_executable(map_obstacle_layer_node
  src/map_obstacle_layer_node.cpp
)

target_link_libraries(map_obstacle_layer_node
  robot_sim
)


install(
  DIRECTORY "include/"
  DESTINATION include
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()


install(DIRECTORY
  launch
  config
  DESTINATION share/${PROJECT_NAME}/
)

ament_auto_package()
