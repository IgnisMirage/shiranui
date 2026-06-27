cmake_minimum_required(VERSION 3.8)
project(mid360_driver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Mid360RawPacket.msg"
  "msg/Mid360Packet.msg"
  DEPENDENCIES std_msgs
)

include_directories(include)

add_library(${PROJECT_NAME}_core
  src/crc.cpp
  src/sdk_protocol.cpp
  src/packet_codec.cpp
  src/mid360_driver.cpp
)

ament_target_dependencies(${PROJECT_NAME}_core
  rclcpp
  std_msgs
  sensor_msgs
)

rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp)
target_link_libraries(${PROJECT_NAME}_core "${cpp_typesupport_target}")

add_executable(mid360_driver_node src/mid360_driver_node.cpp)
target_link_libraries(mid360_driver_node ${PROJECT_NAME}_core)
ament_target_dependencies(mid360_driver_node rclcpp)

add_executable(mid360_packet_to_cloud_node src/mid360_packet_to_cloud_node.cpp)
target_link_libraries(mid360_packet_to_cloud_node ${PROJECT_NAME}_core)
ament_target_dependencies(mid360_packet_to_cloud_node rclcpp sensor_msgs)

install(TARGETS ${PROJECT_NAME}_core mid360_driver_node mid360_packet_to_cloud_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
