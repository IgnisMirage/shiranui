cmake_minimum_required(VERSION 3.8)
project(tg15_driver)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_srvs REQUIRED)

# Include directories
include_directories(include)

# Create the TG15 driver library
add_library(${PROJECT_NAME} SHARED
  src/tg15_driver.cpp
)

ament_target_dependencies(${PROJECT_NAME}
  rclcpp
  rclcpp_components
  sensor_msgs
  std_srvs
)

# Create the node executable
add_executable(${PROJECT_NAME}_node
  src/tg15_driver_node.cpp
)

target_link_libraries(${PROJECT_NAME}_node ${PROJECT_NAME})

ament_target_dependencies(${PROJECT_NAME}_node
  rclcpp
  sensor_msgs
  std_srvs
)

# Install the library
install(TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# Install the executable
install(TARGETS ${PROJECT_NAME}_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install header files
install(DIRECTORY include/
  DESTINATION include
)

# Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
  FILES_MATCHING PATTERN "*.py"
  PATTERN "*.xml"
)

# Install config files
install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME}
  FILES_MATCHING PATTERN "*.yaml"
)

# Export dependencies
ament_export_dependencies(
  rclcpp
  rclcpp_components
  sensor_msgs
  std_srvs
)

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})

# Register the component
rclcpp_components_register_nodes(${PROJECT_NAME} "tg15_driver::TG15Driver")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
