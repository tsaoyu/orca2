cmake_minimum_required(VERSION 3.5)
project(orca_base)

# Default to C++14
if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif ()

# Emulate colcon by providing paths to other projects in the workspace
if ($ENV{CLION_IDE})
  set(fiducial_vlam_msgs_DIR "${PROJECT_SOURCE_DIR}/../../../install/fiducial_vlam_msgs/share/fiducial_vlam_msgs/cmake")
  set(orca_msgs_DIR "${PROJECT_SOURCE_DIR}/../../../install/orca_msgs/share/orca_msgs/cmake")
  set(ros2_shared_DIR "${PROJECT_SOURCE_DIR}/../../../install/ros2_shared/share/ros2_shared/cmake")
endif ()

find_package(ament_cmake REQUIRED)
find_package(fiducial_vlam_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(orca_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(ros2_shared REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(visualization_msgs REQUIRED)

include_directories(
  include
)

#=============
# Base node
#=============

add_executable(
  base_node
  src/base_node.cpp
  src/mission.cpp
  src/motion.cpp
  src/util.cpp
)

ament_target_dependencies(
  base_node
  fiducial_vlam_msgs
  orca_msgs
  nav_msgs
  rclcpp
  ros2_shared
  sensor_msgs
  tf2
  tf2_ros
  visualization_msgs
)

#=============
# Test node
#=============

add_executable(
  test_node
  src/test_node.cpp
)

ament_target_dependencies(
  test_node
  rclcpp
  sensor_msgs
)

ament_export_include_directories(include)

install(TARGETS base_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY include/${PROJECT_NAME} DESTINATION include)

ament_package()