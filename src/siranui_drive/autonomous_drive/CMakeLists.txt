cmake_minimum_required(VERSION 3.8)
project(autonomous_drive)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(PCL REQUIRED COMPONENTS common io filters)
find_package(yaml-cpp REQUIRED)
find_package(PNG REQUIRED)

add_executable(map_publisher_node src/map_publisher_node.cpp)
ament_target_dependencies(map_publisher_node rclcpp nav_msgs)
target_link_libraries(map_publisher_node yaml-cpp PNG::PNG)

install(TARGETS map_publisher_node
  DESTINATION lib/${PROJECT_NAME}
)

add_executable(map_pointcloud_publisher_node src/map_pointcloud_publisher_node.cpp)
target_include_directories(map_pointcloud_publisher_node PRIVATE ${PCL_INCLUDE_DIRS})
ament_target_dependencies(map_pointcloud_publisher_node rclcpp sensor_msgs pcl_conversions)
target_link_libraries(map_pointcloud_publisher_node ${PCL_LIBRARIES})

install(TARGETS map_pointcloud_publisher_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  config
  launch
  map
  DESTINATION share/${PROJECT_NAME}/
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
