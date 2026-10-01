cmake_minimum_required(VERSION 3.8)
project(ndt_omp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PCL is unstable in a debug build (upstream ndt_omp pins RELEASE for the same reason), and
# NDT in a debug build cannot keep up with a 10 Hz scan.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(PCL REQUIRED COMPONENTS common io filters search registration)
find_package(OpenMP REQUIRED)

# Instantiate the PCL templates (PCLBase, KdTreeFLANN, VoxelGrid, ...) in this package instead of
# linking PCL's precompiled instances: the arm64 CI build could not resolve those instances.
add_compile_definitions(PCL_NO_PRECOMPILE)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/NdtStatus.msg"
  DEPENDENCIES std_msgs
)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

# Vendored OpenMP NDT (koide3/ndt_omp). Do not edit include/pclomp or src/pclomp.
add_library(pclomp STATIC
  src/pclomp/voxel_grid_covariance_omp.cpp
  src/pclomp/ndt_omp.cpp
)
target_include_directories(pclomp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  ${PCL_INCLUDE_DIRS}
)
target_compile_definitions(pclomp PUBLIC ${PCL_DEFINITIONS})
target_link_libraries(pclomp ${PCL_LIBRARIES} OpenMP::OpenMP_CXX)

add_executable(ndt_omp_node src/ndt_omp_node.cpp)
target_link_libraries(ndt_omp_node
  pclomp
  ${cpp_typesupport_target}
  ${PCL_LIBRARIES}
)
ament_target_dependencies(ndt_omp_node
  rclcpp
  std_msgs
  sensor_msgs
  nav_msgs
  geometry_msgs
  pcl_conversions
  tf2
  tf2_ros
  tf2_eigen
)

install(TARGETS ndt_omp_node
  DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
