//
// Created by usrg on 7/10/21.
//

#include "nif_objects_tracker_nodes/objects_tracker_nodes.h"
#include "nif_common/constants.h"

using nif::perception::IMMObjectTrackerNode;

IMMObjectTrackerNode::IMMObjectTrackerNode(const std::string &node_name_)
    : Node(node_name_, rclcpp::NodeOptions().use_intra_process_comms(true)) {
  std::string package_share_directory;
  try {
    // This value shouldn't be used, it's as a backup if a config param is
    // missing.
    package_share_directory = ament_index_cpp::get_package_share_directory(
        "nif_objects_tracker_nodes");
  } catch (std::exception e) {
    RCLCPP_FATAL(this->get_logger(), "Can't get package_share_directory");
  }
  package_share_directory = package_share_directory.append("/");
  this->declare_parameter("tracking_config_file_path",
                          tracker_config_file_path);
  tracker_config_file_path =
      this->get_parameter("tracking_config_file_path").as_string();
  tracker_config_file_path.insert(0, package_share_directory);

  tracked_output_publisher =
      this->create_publisher<nif_msgs::msg::Perception3DArray>(
          "out_detected_object", nif::common::constants::QOS_SENSOR_DATA);

  tracked_output_vis_publisher =
      this->create_publisher<visualization_msgs::msg::MarkerArray>(
          "output_vis", nif::common::constants::QOS_SENSOR_DATA);

  detection_result_subscription =
      this->create_subscription<nif_msgs::msg::Perception3DArray>(
          "in_perception", nif::common::constants::QOS_SENSOR_DATA,
          std::bind(&IMMObjectTrackerNode::detectionCallback, this,
                    std::placeholders::_1));

  ego_odom_subscription = this->create_subscription<nav_msgs::msg::Odometry>(
      "in_ego_odom", nif::common::constants::QOS_SENSOR_DATA,
      std::bind(&IMMObjectTrackerNode::egoOdomCallback, this,
                std::placeholders::_1));

  rof_subscription = this->create_subscription<deep_orange_msgs::msg::RestOfFieldReport>(
    "/raptor_dbw_interface/rest_of_field_report", nif::common::constants::QOS_SENSOR_DATA,
      std::bind(&IMMObjectTrackerNode::rofCallback, this,
                std::placeholders::_1));

  tracker_ptr = std::make_shared<ImmUkfPda>(tracker_config_file_path);
  // success to initialize the tracker
}

void IMMObjectTrackerNode::detectionCallback(
    const nif_msgs::msg::Perception3DArray::UniquePtr msg) {

  // TODO : Set the latest ego position to the tracker
  tracker_ptr->setEgoOdom(ego_odom);
  // TODO : Set the detection result to the tracker
  // TODO : TOPIC TYPE CONVERSION : Perception3DArray to DetectionArray
  nif_msgs::msg::DetectedObjectArray tmp_array;
  tmp_array.header = msg->header;
  for (int det_idx = 0; det_idx < msg->perception_list.size(); det_idx++) {
    nif_msgs::msg::DetectedObject tmp;

    tmp.header = msg->perception_list[det_idx].header;
    tmp.pose.position.x =
        msg->perception_list[det_idx].detection_result_3d.center.position.x;
    tmp.pose.position.y =
        msg->perception_list[det_idx].detection_result_3d.center.position.y;
    tmp.pose.position.z =
        msg->perception_list[det_idx].detection_result_3d.center.position.z;

    tmp_array.objects.push_back(tmp);
  }

  tracked_objects = tracker_ptr->getTrackedResult(tmp_array);
  tracked_objects.header = msg->header;

  bool rof_healthy = rof_received && this->now() - rof_last_update < rclcpp::Duration(2, 0);
  for (auto&& obj : tracked_objects.perception_list) {
    obj.obj_velocity_in_global.linear.x = rof_healthy ? rof.comp_speed_kph / 3.6 : obj.obj_velocity_in_global.linear.x;

    // TODO WARNING this transform is not always correct
    obj.obj_velocity_in_local.linear.x = obj.obj_velocity_in_global.linear.x - ego_odom.twist.twist.linear.x;
  }

  // @DEBUG
  if (tracked_objects.perception_list.size() > 1)
  {
    rclcpp::Clock clock;
    RCLCPP_WARN_THROTTLE(this->get_logger(), clock, 1.0, "ObjectTracker outputs more than one track.");
  }

  // @DEBUG probably not the best option, but makes prediction stable
  if (!tracked_objects.perception_list.empty()) 
    tracked_output_publisher->publish(tracked_objects);

  // Visualization
  visualization_msgs::msg::MarkerArray vis_marker_array;
  for(int tracked_obj_idx = 0; tracked_obj_idx < tracked_objects.perception_list.size(); tracked_obj_idx++){
    visualization_msgs::msg::Marker vis_marker;

    vis_marker.header = tracked_objects.header; 
    vis_marker.id = tracked_obj_idx;
    vis_marker.action = visualization_msgs::msg::Marker::ADD;
    vis_marker.pose.position.x = tracked_objects.perception_list[tracked_obj_idx].detection_result_3d.center.position.x;
    vis_marker.pose.position.y = tracked_objects.perception_list[tracked_obj_idx].detection_result_3d.center.position.y;
    vis_marker.pose.position.z = 1;
    vis_marker.pose.orientation.x = tracked_objects.perception_list[tracked_obj_idx].detection_result_3d.center.orientation.x;
    vis_marker.pose.orientation.y = tracked_objects.perception_list[tracked_obj_idx].detection_result_3d.center.orientation.y;
    vis_marker.pose.orientation.z = tracked_objects.perception_list[tracked_obj_idx].detection_result_3d.center.orientation.z;
    vis_marker.pose.orientation.w = tracked_objects.perception_list[tracked_obj_idx].detection_result_3d.center.orientation.w;
    vis_marker.scale.x = 1.0 + tracked_objects.perception_list[tracked_obj_idx].obj_velocity_in_local.linear.x;
    vis_marker.scale.y = 1.0;
    vis_marker.scale.z = 1.0;
    vis_marker.color.r = double((tracked_obj_idx + 1.0) / tracked_objects.perception_list.size());
    vis_marker.color.g = 0;
    vis_marker.color.b = 0;
    vis_marker.color.a = 1.0;

    vis_marker.lifetime = rclcpp::Duration(0.1e+9);
    vis_marker_array.markers.push_back(vis_marker);
  }
  tracked_output_vis_publisher->publish(vis_marker_array);
}

void IMMObjectTrackerNode::egoOdomCallback(
    const nav_msgs::msg::Odometry::SharedPtr msg) {
  ego_odom = *msg;
}

void IMMObjectTrackerNode::rofCallback(
    const deep_orange_msgs::msg::RestOfFieldReport::SharedPtr msg) {
  rof = *msg;
  rof_received = true;
  rof_last_update  = this->now();
}