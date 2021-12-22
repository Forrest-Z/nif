//  Copyright (c) 2021 Unmanned System Research Group @ KAIST
//  Author:

//
// Created by usrg on 6/23/21.
//

#include "nif_geofence_filter_nodes/geofence_filter_node.h"

#include "rcutils/error_handling.h"
#include <cstdio>
#include <iostream>
#include <memory>

int32_t main(int32_t argc, char **argv) {
  rclcpp::init(argc, argv);

  using namespace nif::common::constants;
  using namespace nif::perception;

  const char *node_name = "geofence_filter_node";
  rclcpp::Node::SharedPtr nd;

  try {
    RCLCPP_INFO(rclcpp::get_logger(LOG_MAIN_LOGGER_NAME),
                "Instantiating GeofenceFilterNode with name: %s", node_name);
    nd = std::make_shared<GeofenceFilterNode>(node_name);

  } catch (std::exception &e) {
    RCLCPP_FATAL(rclcpp::get_logger(LOG_MAIN_LOGGER_NAME),
                 "FATAL ERROR during GeofenceFilterNode initialization: ABORTING.\n%s",
                 e.what());
    return -1;
  }
  
  rclcpp::spin(nd);
  rclcpp::shutdown();

  RCLCPP_INFO(rclcpp::get_logger(LOG_MAIN_LOGGER_NAME),
              "Shutting down %s [GeofenceFilterNode]", node_name);

  return 0;
}