//
// Created by usrg on 8/3/21.
//

#ifndef ROS2MASTER_SYSTEM_STATUS_MANAGER_NODE_H
#define ROS2MASTER_SYSTEM_STATUS_MANAGER_NODE_H

#include <rclcpp/rclcpp.hpp>
#include "nif_common/types.h"
#include "nif_msgs/msg/localization_status.hpp"
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/int32.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_srvs/srv/trigger.hpp>
#include "nif_msgs/srv/register_node_status.hpp"
#include "deep_orange_msgs/msg/diagnostic_report.hpp"

using nif::common::NodeStatusCode;
using nif::common::NodeType;
using nif::common::SystemStatusCode;

using t_record_index = unsigned int;


namespace nif {
namespace system {

const rclcpp::Duration NODE_DEAD_TIMEOUT(500000000); // 0.5s

struct NodeStatusRecord {
  nif::common::msgs::NodeStatus::SharedPtr node_status;

};

class SystemStatusManagerNode : public rclcpp::Node {

public:
  explicit SystemStatusManagerNode(const std::string &node_name);

private:
  nif::common::types::t_node_id next_node_id = 1000;

  nif::common::msgs::SystemStatus system_status_msg;

  bool is_system_healthy = false;

  common::SystemStatusCode system_status_code = common::SYSTEM_NOT_INITIALIZED;

  /**
  * Race Control input last update time.
  */
  rclcpp::Time mission_update_time;

  bool has_mission = false;

  unsigned int mode = 255;
//  TODO make parameter
  const int max_counter_drop = 75; // 2.5s at 30hz
  int t = 0;

  const int default_counter = 502;
  int counter_joy_prev = default_counter;
  int counter_joy = default_counter;
  bool joy_emergency_stop = false;
  bool recovery_enabled = true;
  bool comms_heartbeat_ok = false;

  // double localization_error = 1000.0;
  nif_msgs::msg::LocalizationStatus localization_status;
  bool has_localization_status = false;

  rclcpp::Time localization_status_last_update = rclcpp::Time();

  rclcpp::Duration timeout_mission = rclcpp::Duration(1, 0);
  rclcpp::Duration timeout_localization_status = rclcpp::Duration(0, 500000000);

  // Mission and safe localization parameters
  double velocity_zero = 0.0;
  double safeloc_threshold_stop = 0.0;
  double safeloc_threshold_slow_down = 0.0;
  double safeloc_velocity_slow_down_max = 0.0;
  double safeloc_velocity_slow_down_min = 0.0;

  rclcpp::Time ll_diagnostic_report_last_update = rclcpp::Time();
  bool is_ll_healthy = false;
  bool has_ll_diagnostic_report = false;

  bool hasLocalizationStatus();

  /**
   * SystemStatus publisher. Publishes the latest system_status message, after
   * checking each node status.
   */
  rclcpp::Publisher<nif::common::msgs::SystemStatus>::SharedPtr system_status_pub;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr joy_emergency_pub;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr hb_emergency_pub;

  rclcpp::Publisher<nif::common::msgs::SystemStatus>::SharedPtr system_status_telem_pub;

  rclcpp::Subscription<nif::common::msgs::OverrideControlCmd>::SharedPtr joystick_sub;
  rclcpp::Subscription<nif_msgs::msg::LocalizationStatus>::SharedPtr localization_status_sub;
  
  std::vector<
      rclcpp::Subscription<nif::common::msgs::NodeStatus>::SharedPtr> node_statuses_subs;
  
  rclcpp::Subscription<nif::common::msgs::MissionStatus>::SharedPtr mission_status_sub;

  rclcpp::Subscription<deep_orange_msgs::msg::DiagnosticReport>::SharedPtr ll_diagnostic_report_sub;

  rclcpp::TimerBase::SharedPtr system_status_timer;
  rclcpp::TimerBase::SharedPtr comms_heartbeat_timer;

  rclcpp::Service<nif_msgs::srv::RegisterNodeStatus>::SharedPtr
      register_node_service;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr recovery_service;

  //  Node statuses storage and indices
  std::vector<std::shared_ptr<NodeStatusRecord>> node_status_records{};

  std::unordered_map<nif::common::types::t_node_id, t_record_index > status_index_by_id{};

  std::unordered_map<std::string, t_record_index> status_index_by_name{};

  std::unordered_map<NodeType,
                     std::unique_ptr<
                         std::vector<t_record_index>>> status_indices_by_type{};


  void subscribeNodeStatus(const std::string & topic_name);

  void registerNodeServiceHandler(
      const std::shared_ptr<rmw_request_id_t> request_header,
      const nif_msgs::srv::RegisterNodeStatus::Request::SharedPtr request,
      nif_msgs::srv::RegisterNodeStatus::Response::SharedPtr response);

  nif::common::types::t_node_id newStatusRecord(const nif_msgs::srv::RegisterNodeStatus::Request::SharedPtr request);
  void nodeStatusUpdate(const nif::common::msgs::NodeStatus::SharedPtr msg);

  void systemStatusTimerCallback();
  void commsHeartbeatTimerCallback();

  void joystickCallback(const nif::common::msgs::OverrideControlCmd::SharedPtr msg);
  void localizationStatusCallback(const nif_msgs::msg::LocalizationStatus::SharedPtr msg);
  void missionCallback(const nif::common::msgs::MissionStatus::UniquePtr msg);
  void llDiagnosticReportCallback(const deep_orange_msgs::msg::DiagnosticReport::UniquePtr msg);

  void recoveryServiceHandler(
          const std::shared_ptr<rmw_request_id_t> request_header,
          const std_srvs::srv::Trigger::Request::SharedPtr request,
          std_srvs::srv::Trigger::Response::SharedPtr response);

  /**
   * System Healthy state machine, with boolean output
   * @return boolean system health state. true if system health is nominal.
   */
  bool isSystemHealthy();

  /**
   * System status state machine, with status output code.
   * @return system status code
   */
  common::SystemStatusCode getSystemStatusCode();
  void nodeStatusesAgeCheck();

  bool commsHeartbeatOk();
  bool localizationOk();

  rcl_interfaces::msg::SetParametersResult
  parametersCallback(
          const std::vector<rclcpp::Parameter> &vector);

  rclcpp::Duration timeout_node_inactive = rclcpp::Duration(1, 0);
  OnSetParametersCallbackHandle::SharedPtr parameters_callback_handle;

  /**
   * Limit the maximum speed according to the current localization error.
   * 
   * @param max_vel_mps maximum allowed speed in m/s
   * @return max_vel_mps linearly clipped according to the last localization error and the defined set of parameters.
  **/ 
  void processSafelocVelocity(double & max_vel_mps);
};

} // namespace system
} // namespace nif

#endif // ROS2MASTER_SYSTEM_STATUS_MANAGER_NODE_H
