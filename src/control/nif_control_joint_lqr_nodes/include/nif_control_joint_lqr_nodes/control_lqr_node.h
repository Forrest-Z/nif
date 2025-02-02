//
// Created by usrg on 8/22/21.
//

#ifndef ROS2MASTER_CONTROL_LQR_NODE_H
#define ROS2MASTER_CONTROL_LQR_NODE_H

/**
 * @brief path following node
 **/
#include "nif_control_common_nodes/i_controller_node.h"

#include "deep_orange_msgs/msg/pt_report.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "nav_msgs/msg/path.hpp"
#include "raptor_dbw_msgs/msg/wheel_speed_report.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/float32.hpp"
#include "std_msgs/msg/int32.hpp"
#include "std_msgs/msg/u_int8.hpp"

#include "nif_control_joint_lqr_nodes/lqr/joint_lqr.h"
#include "nif_control_joint_lqr_nodes/utils/joint_lqr_ros.h"
#include "nif_control_joint_lqr_nodes/utils/pure_pursuit_tracker.h"

#include "nif_control_common/camber_compensator.hpp"

namespace nif {
namespace control {

class ControlLQRNode : public nif::control::IControllerNode {
public:
  explicit ControlLQRNode(const std::string &node_name);

  void publishSteerAccelDiagnostics(
      bool lqr_command_valid, bool valid_path, bool valid_odom,
      bool valid_wpt_distance, bool valid_target_position,
      double lqr_steering_command, double lqr_accel_command,
      double track_distance, unsigned int lqr_tracking_idx,
      geometry_msgs::msg::PoseStamped lqr_track_point,
      joint_lqr::lqr::JointLQR::ErrorMatrix lqr_err_cog,
      joint_lqr::lqr::JointLQR::ErrorMatrix lqr_err,
      double desired_velocity_mps);

  /** ROS Callbacks / Subscription Interface **/
  void afterReferencePathCallback() override {
    lqr_tracking_idx_ = 0; // Reset Tracking
  }

  // void desiredVxCallback(const std_msgs::msg::Float32::SharedPtr msg) {
  //   desired_vx_ = msg->data;
  // }

  // ACC
  void accCMDCallback(const std_msgs::msg::Float32::SharedPtr msg) {
    acc_accel_cmd_mpss = msg->data;
  }

  void velocityCallback(
      const raptor_dbw_msgs::msg::WheelSpeedReport::SharedPtr msg) {
    current_speed_ms_ = (msg->front_left + msg->front_right) * 0.5 *
                        nif::common::constants::KPH2MS;
  }

  void
  directDesiredVelocityCallback(const std_msgs::msg::Float32::SharedPtr msg) {
    direct_desired_velocity_ = msg->data;
  }

  void steeringCallback(const std_msgs::msg::Float32::SharedPtr msg) {
    override_steering_target_ = msg->data;
  }

  void throttleCallback(const std_msgs::msg::Float32::SharedPtr msg) {
    override_throttle_target_ = msg->data;
  }

  void brakeCallback(const std_msgs::msg::Float32::SharedPtr msg) {
    override_brake_target_ = msg->data;
  }

  void gearCallback(const std_msgs::msg::UInt8::SharedPtr msg) {
    override_gear_target_ = msg->data;
  }

  void ptReportCallback(const deep_orange_msgs::msg::PtReport::SharedPtr msg) {
    current_gear_ = msg->current_gear;
    current_engine_speed_ = msg->engine_rpm;
    engine_running_ = (msg->engine_rpm > 500) ? true : false;
  }

private:
  //! Debug Interface
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr lqr_command_valid_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr
      lqr_valid_conditions_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr
      lqr_steering_command_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr lqr_accel_command_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr track_distance_pub_;
  rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr lqr_tracking_idx_pub_;

  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr
      lqr_tracking_point_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr lqr_error_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr
      lqr_desired_velocity_mps_pub_;
  //! Command Publishers
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr steering_command_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr throttle_command_pub_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr brake_command_pub_;
  rclcpp::Publisher<std_msgs::msg::UInt8>::SharedPtr gear_command_pub_;
  //! Input Data
  rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr desired_vx_sub_;
  rclcpp::Subscription<raptor_dbw_msgs::msg::WheelSpeedReport>::SharedPtr
      velocity_sub_;
  rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr
      direct_desired_velocity_sub;

  rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr steering_sub_;
  rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr throttle_sub_;
  rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr brake_sub_;
  rclcpp::Subscription<std_msgs::msg::UInt8>::SharedPtr gear_sub_;
  rclcpp::Subscription<deep_orange_msgs::msg::PtReport>::SharedPtr
      pt_report_sub_;
  // ACC
  rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr acc_sub_;

  //! Lateral LQR Controller
  joint_lqr::lqr::JointLQR::Ptr joint_lqr_;

  //! Control command published by solve()
  nif::common::msgs::ControlCmd::SharedPtr control_cmd;

  // camber compensator
  std::shared_ptr<CamberCompensator> m_camber_manager_ptr;

  //! Current Vehicle State
  double current_speed_ms_;
  double direct_desired_velocity_;
  uint8_t current_gear_;
  uint8_t current_engine_speed_ = 0.0;
  bool engine_running_;
  double last_steering_command_ = 0.0;
  double last_accel_command_ = 0.0;

  //! Desired Velocity from Velocity Planner
  double desired_vx_;

  // ACC
  double acc_accel_cmd_mpss;

  //! LQR Tracking State
  unsigned int lqr_tracking_idx_ = 0;

  //! Manual Overrides for when auto mode is disabled
  double override_steering_target_ = 0.0;
  double override_brake_target_ = 0.0;
  double override_throttle_target_ = 0.0;
  uint8_t override_gear_target_ = 0;

  //! Misc. Parameters (see notes in constructor)
  double max_steering_angle_deg_;
  double steering_auto_override_deg_;
  double steering_units_multiplier_;
  double pure_pursuit_min_dist_m_;
  double pure_pursuit_max_dist_m_;
  double pure_pursuit_1st_vel_ms_;
  double pure_pursuit_max_max_dist_m_;
  double pure_pursuit_k_vel_m_ms_;
  bool use_tire_velocity_;
  double odometry_timeout_sec_;
  double path_timeout_sec_;
  double steering_max_ddeg_dt_;
  double des_accel_max_da_dt_;
  double des_deccel_max_da_dt_;
  double m_path_min_length_m;
  bool invert_steering_;
  bool m_use_mission_max_vel_;

  // ACC enable ros param
  bool m_use_acc;

  rcl_interfaces::msg::SetParametersResult
  parametersCallback(const std::vector<rclcpp::Parameter> &vector);

  nif::common::msgs::ControlCmd::SharedPtr solve() override;

}; /* class PathFollowerNode */

} // namespace control
} // namespace nif

#endif // ROS2MASTER_CONTROL_LQR_NODE_H
