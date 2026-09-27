cmake_minimum_required(VERSION 3.8)
project(ndt_localizer)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(PCL REQUIRED COMPONENTS common io filters)
find_package(ndt_omp REQUIRED)

add_executable(ndt_localizer_node src/ndt_localizer_node.cpp)

target_include_directories(ndt_localizer_node PRIVATE
  ${PCL_INCLUDE_DIRS}
)
if(ndt_omp_INCLUDE_DIRS)
  target_include_directories(ndt_localizer_node PRIVATE ${ndt_omp_INCLUDE_DIRS})
endif()

ament_target_dependencies(ndt_localizer_node
  rclcpp
  sensor_msgs
  geometry_msgs
  nav_msgs
  std_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
  tf2_eigen
  pcl_conversions
  ndt_omp
)

target_link_libraries(ndt_localizer_node
  ${PCL_LIBRARIES}
  ${ndt_omp_LIBRARIES}
)

install(TARGETS ndt_localizer_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
