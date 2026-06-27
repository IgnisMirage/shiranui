cmake_minimum_required(VERSION 3.8)
project(stanley_planner)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()


ament_auto_add_library(stanley_planner SHARED
  src/stanley_planner_component.cpp
)

rclcpp_components_register_node(stanley_planner
  PLUGIN "stanley_planner::StanleyNode"
  EXECUTABLE stanley_planner_component
)


ament_auto_add_executable(stanley_planner_node
  src/stanley_planner_node.cpp
)

target_link_libraries(stanley_planner_node
  stanley_planner
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
