cmake_minimum_required(VERSION 3.5)
project(nif_localization_nodes)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(PCL REQUIRED)

find_path(
  GeographicLib_INCLUDE_DIR GeographicLib/Config.h
  PATH_SUFFIXES GeographicLib
)
set(GeographicLib_INCLUDE_DIRS ${GeographicLib_INCLUDE_DIR})

find_library(GeographicLib_LIBRARIES
  NAMES Geographic
)

include_directories(
  include
  ${GeographicLib_INCLUDE_DIRS}
  ${PCL_INCLUDE_DIRS}
)
add_definitions(${GeographicLib_DEFINITIONS})

# find dependencies
find_package(ament_cmake_auto REQUIRED)
# find_package(Eigen3 3.3 REQUIRED NO_MODULE)
ament_auto_find_build_dependencies()

# Library
ament_auto_add_library(geodetic_conv SHARED
        lib/geodetic_conv.cpp)

ament_auto_add_library(ekf_localizer_lib SHARED
        lib/c_ekf.cpp)

ament_auto_add_library(globalmap_loader SHARED
        src/load_globalmap_node.cpp)
target_link_libraries(globalmap_loader
        ${PCL_LIBRARIES}
)

ament_auto_add_library(ekf_localizer SHARED
        src/localization_ekf_node.cpp)

target_link_libraries(ekf_localizer
        ekf_localizer_lib
        geodetic_conv
)

ament_auto_add_library(load_geofence SHARED
        src/load_geofence_node.cpp)
target_link_libraries(load_geofence
        ${PCL_LIBRARIES}
)

ament_auto_add_library(resilient_localize SHARED
        src/resilient_localization_node.cpp)
target_link_libraries(resilient_localize
        ${PCL_LIBRARIES}
)

ament_auto_add_executable(${PROJECT_NAME}_exe
        src/localization_main.cpp)
target_link_libraries(${PROJECT_NAME}_exe
        ekf_localizer
        load_geofence
        resilient_localize
        globalmap_loader
)

ament_auto_add_executable(map_generator_node
        src/map_generator_node.cpp)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  add_ros_test(
          test/localization_test.launch.py
          TIMEOUT "30"
  )
  ament_add_gtest(${PROJECT_NAME}_test
          test/gtest_main.cpp
          )

  target_include_directories(${PROJECT_NAME}_test PRIVATE "include")
  target_link_libraries(${PROJECT_NAME}_test ekf_localizer_lib)
  ament_target_dependencies(${PROJECT_NAME}_test
          "rclcpp")
endif()

ament_auto_package(INSTALL_TO_SHARE
        launch
        map)
