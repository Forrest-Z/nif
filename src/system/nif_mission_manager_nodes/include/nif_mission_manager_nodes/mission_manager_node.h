//  Copyright (c) 2021 Unmanned System Research Group @ KAIST
//  Author: Andrea Finazzi

#ifndef ROS2MASTER_MISSION_MANAGER_NODE_H
#define ROS2MASTER_MISSION_MANAGER_NODE_H

#include "nif_common_nodes/i_base_synchronized_node.h"
#include "nif_mission_manager/mission_parser.h"
#include "raptor_dbw_msgs/msg/wheel_speed_report.hpp"

using nif_msgs::msg::MissionStatus;
using nif_msgs::msg::MissionStatus;

namespace nif {
namespace system {


class MissionManagerNode : public nif::common::IBaseSynchronizedNode 
{

public:
  explicit MissionManagerNode(const std::string &node_name);

private:
  MissionStatus mission_status_msg;

  /**
  * Race Control input from the race control interface.
  * It's automatically stored by its callback along with race_control_status_update_time.
  */
  nif::common::msgs::RCFlagSummary rc_flag_summary;
  
  /**
  * Race Control input last update time.
  */
  rclcpp::Time rc_flag_summary_update_time;
  bool has_rc_flag_summary = false;
  rclcpp::Duration timeout_rc_flag_summary = rclcpp::Duration(10, 0);

  /**
  * Current ego vehicle velocity
  */
  double current_velocity_mps;
  rclcpp::Time current_velocity_update_time;
  bool has_current_velocity = false;
  rclcpp::Duration timeout_current_velocity = rclcpp::Duration(1, 0);

  // Mission and safe localization parameters
  double velocity_zero = 0.0;
  double velocity_max = 0.0;
  double velocity_pit_in = 0.0;
  double velocity_pit_out = 0.0;
  double velocity_slow_drive = 0.0;
  double safeloc_threshold_stop = 0.0;
  double safeloc_threshold_slow_down = 0.0;
  double safeloc_velocity_slow_down_max = 0.0;
  double safeloc_velocity_slow_down_min = 0.0;

  bool is_system_startup = true;

  nif::system::MissionsDescription missions_description;
  
  /**
   * SystemStatus publisher. Publishes the latest system_status message, after
   * checking each node status.
   */
  rclcpp::Publisher<MissionStatus>::SharedPtr mission_status_pub;

  rclcpp::Subscription<nif::common::msgs::RCFlagSummary>::SharedPtr rc_flag_summary_sub;
  rclcpp::Subscription<raptor_dbw_msgs::msg::WheelSpeedReport>::SharedPtr velocity_sub;

  void RCFlagSummaryCallback(const nif::common::msgs::RCFlagSummary::UniquePtr msg);
  void velocityCallback(
    const raptor_dbw_msgs::msg::WheelSpeedReport::SharedPtr msg);
  
  /**
   * Mission Status state machine.
   * @return the mission encoding.
   */
  MissionStatus::_mission_status_code_type getMissionStatusCode();
  MissionStatus::_mission_status_code_type getMissionVehFlagNull(); 
  rcl_interfaces::msg::SetParametersResult
  parametersCallback(
          const std::vector<rclcpp::Parameter> &vector);
  OnSetParametersCallbackHandle::SharedPtr parameters_callback_handle;

  double getMissionMaxVelocityMps(MissionStatus::_mission_status_code_type);

  /**
   * Return a valid mission code based on the current status and the input next_mission.
   */ 
  MissionStatus::_mission_status_code_type validateMissionTransition(
    MissionStatus::_mission_status_code_type next_mission);

  bool isDataOk() {
    auto now = this->now();

    bool rc_ok = this->has_rc_flag_summary && (now - this->rc_flag_summary_update_time) <= timeout_rc_flag_summary;
    bool velocity_ok = this->has_current_velocity && (now - this->current_velocity_update_time) <= this->timeout_current_velocity;
    bool odometry_ok = this->hasEgoOdometry() && (now - this->getEgoOdometryUpdateTime()) <= rclcpp::Duration(1, 0); // TODO make global parameter
    
    return rc_ok && velocity_ok && odometry_ok;
  };

  bool missionIs(MissionStatus::_mission_status_code_type mission) {
    return this->mission_status_msg.mission_status_code == mission;
  }

  void run() final;
};

} // namespace system
} // namespace nif

#endif // ROS2MASTER_MISSION_MANAGER_NODE_H
