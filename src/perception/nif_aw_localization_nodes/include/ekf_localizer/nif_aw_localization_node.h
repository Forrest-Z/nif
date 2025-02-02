//  Copyright (c) 2021 Unmanned System Research Group @ KAIST
//  Author: Daegyu Lee

//
// Created by USRG on 9/14/21.
//

#ifndef AW_EKF_LOCALIZER_NODE_H
#define AW_EKF_LOCALIZER_NODE_H

#include "nif_common/types.h"
#include "nif_common_nodes/i_base_node.h"
#include "nif_localization_minimal/localization_minimal.h"

#include <rclcpp/rclcpp.hpp>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <geometry_msgs/msg/twist_with_covariance_stamped.hpp>
#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <message_filters/synchronizer.h>
#include <message_filters/time_synchronizer.h>
#include <nav_msgs/msg/odometry.hpp>
#include <novatel_oem7_msgs/msg/bestpos.hpp>
#include <novatel_oem7_msgs/msg/bestvel.hpp>
#include <novatel_oem7_msgs/msg/inspva.hpp>
#include <novatel_oem7_msgs/msg/insstdev.hpp>
#include <novatel_oem7_msgs/msg/inertial_solution_status.hpp>
#include <novatel_oem7_msgs/msg/heading2.hpp>
#include <opencv2/opencv.hpp>
#include <raptor_dbw_msgs/msg/wheel_speed_report.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>
#include <std_msgs/msg/float32.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/utils.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>
#include <nif_msgs/msg/localization_status.hpp>

#include "amathutils_lib/kalman_filter.hpp"
#include "amathutils_lib/time_delay_kalman_filter.hpp"

#include <chrono>
#include <iostream>
#include <vector>

#include <utils/geodetic_conv.h>

struct GPSCorrectionData_t
{
    double x;
    double y;
    double yaw;
    double lat_noise;
    double lon_noise;
    double yaw_noise;
    unsigned int novatel_ins_status;
    unsigned int novatel_bestpos_status;
    bool quality_code_ok = false;
};

struct VehPose_t
{
    double x;
    double y;
    double yaw;
};

class AWLocalizationNode : public nif::common::IBaseNode {

public:
  AWLocalizationNode(const std::string &node_name);
  ~AWLocalizationNode();

private:
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr pub_odom_;
  rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr
      pub_debug_; // !< @brief debug info publisher
  rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr
      pub_measured_pose_; // !< @brief debug measurement pose publisher
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr
      pub_yaw_bias_; // !< @brief ekf estimated yaw bias publisher
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr
      pub_mahalanobisScore; // localization result score
  rclcpp::Publisher<nif_msgs::msg::LocalizationStatus>::SharedPtr
      pub_localization_status;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr
      pub_bestpos_odometry;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr
      pub_top_bestpos_odometry;

  rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr
      sub_initialpose_; // !< @brief initial pose subscriber
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr
      sub_odom_measure_; // !< @brief measurement odom subscriber including
                         // pose
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr
      sub_odom_system_; // !< @brief measurement odom subscriber including
                        // twist
  rclcpp::TimerBase::SharedPtr
      timer_control_; // !< @brief time for ekf calculation callback
  rclcpp::TimerBase::SharedPtr timer_tf_; // !< @brief timer to send transform
  rclcpp::Subscription<novatel_oem7_msgs::msg::BESTPOS>::SharedPtr subBESTPOS;
  rclcpp::Subscription<novatel_oem7_msgs::msg::BESTPOS>::SharedPtr
      subTOPBESTPOS;
  rclcpp::Subscription<novatel_oem7_msgs::msg::INSPVA>::SharedPtr subINSPVA;
  rclcpp::Subscription<novatel_oem7_msgs::msg::INSPVA>::SharedPtr subTOPINSPVA;
  rclcpp::Subscription<novatel_oem7_msgs::msg::BESTVEL>::SharedPtr subBESTVEL;
  rclcpp::Subscription<novatel_oem7_msgs::msg::BESTVEL>::SharedPtr subTOPBESTVEL;
  rclcpp::Subscription<novatel_oem7_msgs::msg::HEADING2>::SharedPtr subHEADING2;
  rclcpp::Subscription<novatel_oem7_msgs::msg::HEADING2>::SharedPtr subTOPHEADING2;
  rclcpp::Subscription<novatel_oem7_msgs::msg::INSSTDEV>::SharedPtr subINSSTDEV;
  rclcpp::Subscription<novatel_oem7_msgs::msg::INSSTDEV>::SharedPtr
      subTOPINSSTDEV;
  rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr subIMUONLY;
  rclcpp::Subscription<std_msgs::msg::Float64>::SharedPtr subtest;
  using SyncPolicyT = message_filters::sync_policies::ApproximateTime<
      sensor_msgs::msg::Imu, raptor_dbw_msgs::msg::WheelSpeedReport>;
  message_filters::Subscriber<sensor_msgs::msg::Imu> sub_filtered_IMU;
  message_filters::Subscriber<raptor_dbw_msgs::msg::WheelSpeedReport>
      sub_filtered_Wheel;

  std::shared_ptr<message_filters::Synchronizer<SyncPolicyT>> m_sync;
  std::unique_ptr<tf2_ros::TransformBroadcaster> broadcaster_;

  nif::localization::utils::GeodeticConverter conv_;

  TimeDelayKalmanFilter ekf_; // !< @brief  extended kalman filter instance.

  GPSCorrectionData_t BestPosBottom;
  GPSCorrectionData_t BestPosTop;

  nif_msgs::msg::LocalizationStatus m_localization_status;

  /* parameters */
  bool show_debug_info_;
  double ekf_rate_;                 // !< @brief  EKF predict rate
  double ekf_dt_;                   // !< @brief  = 1 / ekf_rate_
  double tf_rate_;                  // !< @brief  tf publish rate
  bool enable_yaw_bias_estimation_; // !< @brief  for LiDAR mount error. if
                                    // true, publish /estimate_yaw_bias
  std::string pose_frame_id_;

  std::string measure_odom_name_;
  std::string system_odom_name_;
  std::string final_output_odom_name_;

  int dim_x_;             // !< @brief  dimension of EKF state
  int extend_state_step_; // !< @brief  for time delay compensation
  int dim_x_ex_; // !< @brief  dimension of extended EKF state (dim_x_ *
                 // extended_state_step)

  /* Pose */
  double pose_additional_delay_; // !< @brief  compensated pose delay time =
                                 // (pose.header.stamp - now) +
                                 // !< additional_delay [s]
  double pose_measure_uncertainty_time_; // !< @brief  added for measurement
                                         // covariance
  double
      pose_rate_; // !< @brief  pose rate [s], used for covariance calculation
  double pose_gate_dist_; // !< @brief  pose measurement is ignored if the
                          // maharanobis distance is larger than this
                          // !< value.
  double
      pose_stddev_x_; // !< @brief  standard deviation for pose position x [m]
  double
      pose_stddev_y_; // !< @brief  standard deviation for pose position y [m]
  double pose_stddev_yaw_; // !< @brief  standard deviation for pose position
                           // yaw [rad]
  bool use_pose_with_covariance_;  // !< @brief  use covariance in
                                   // pose_with_covarianve message
  bool use_twist_with_covariance_; // !< @brief  use covariance in
                                   // twist_with_covarianve message

  /* twist */
  double twist_linear_x;          // !< @brief  Vehicle longitudinal velocity.
  double twist_additional_delay_; // !< @brief  compensated delay time =
                                  // (twist.header.stamp - now) +
                                  // additional_delay
                                  // !< [s]
  double twist_rate_; // !< @brief  rate [s], used for covariance calculation
  double twist_gate_dist_; // !< @brief  measurement is ignored if the
                           // maharanobis distance is larger than this value.
  double twist_stddev_vx_; // !< @brief  standard deviation for linear vx
  double twist_stddev_wz_; // !< @brief  standard deviation for angular wx
  double invert_imu;

  /* process noise variance for discrete model */
  double proc_cov_yaw_d_;      // !< @brief  discrete yaw process noise
  double proc_cov_yaw_bias_d_; // !< @brief  discrete yaw bias process noise
  double proc_cov_vx_d_;       // !< @brief  discrete process noise in d_vx=0
  double proc_cov_wz_d_;       // !< @brief  discrete process noise in d_wz=0

  double m_origin_lat;
  double m_origin_lon;

  double m_heading_lowpass_gain;

  double m_dGPS_X;
  double m_dGPS_Y;
  double m_dGPS_Z;
  double m_dGPS_Heading;
  double m_dGPS_Heading_prev;
  double m_dGPS_TOP_Heading;
  double m_dGPS_TOP_Heading_prev;
  double m_heading2_heading_rad;
  double m_top_heading2_heading_rad;
  double m_best_heading_rad;
  double m_dGPS_roll;
  double m_dGPS_TOP_roll; // Note : roll of top and bottom are very different. Add field for novatel top
  double m_prevYaw;
  double m_prevTOPYaw;

  double m_d_TOP_GPS_X;
  double m_d_TOP_GPS_Y;
  double m_d_TOP_GPS_Z;

  bool bBOTTOM_GPS = false;
  bool bTOP_GPS = false;

  bool m_heading_initial_guess_enabled = false;
  bool bBOTTOMGPSHeading = false;
  bool bTOPGPSHeading = false;
  bool bottom_gps_update = false;
  bool top_gps_update = false;
  bool m_heading_initialized = false;
  
  bool m_heading2_valid = false;
  bool m_top_heading2_valid = false;
  bool m_bestvel_bottom_valid = false;
  bool m_bestvel_top_valid = false;
  bool m_heading_error = true;

  bool bUseBestVelForSpeed;
  bool bInitConverged = false;

  bool measure_flag = false;
  bool m_inspva_heading_init = false;
  bool m_top_inspva_heading_init = false;

  bool m_use_inspva_heading;
  double m_bestvel_heading_update_thres;
  double m_heading_heading2_offset_rad;

  double m_dIMU_yaw_rate;
  double m_dVelolcity_X;
  double m_BOTTOM_VEL_X;
  double m_BOTTOM_VEL_Y;
  double m_TOP_VEL_X;
  double m_TOP_VEL_Y;
  double m_final_INSPVA_Vel_Y;

  bool bImuFirstCall;
  double ImuTimeDouble;
  double ImuPrevTimeDouble;

  bool bVehVelocityFirstCall;
  double VehVelocityTimeDouble;
  double VehVelocityPrevTimeDouble;

  double m_stdev_longitude;
  double m_stdev_latitude;
  double m_stdev_azimuth;

  double VehVelocity_dt;
  double m_mahalanobisScore;

  double top_to_bottom_bias_x;
  double top_to_bottom_bias_y;

  double m_testnoise;

  bool enable_tf_publisher;

      rclcpp::Time imu_time_last_update;
  rclcpp::Time bestpos_time_last_update;
  rclcpp::Time top_bestpos_time_last_update;
  rclcpp::Time bestvel_time_last_update;
  rclcpp::Time top_bestvel_time_last_update;
  rclcpp::Time heading2_time_last_update;
  rclcpp::Time top_heading2_time_last_update;
  rclcpp::Duration gps_timeout = rclcpp::Duration(1, 0);

  enum IDX {
    X = 0,
    Y = 1,
    YAW = 2,
    YAWB = 3,
    VX = 4,
    WZ = 5,
  };

  /* for model prediction */
  std::shared_ptr<geometry_msgs::msg::TwistStamped>
      current_twist_ptr_; // !< @brief current measured twist
  std::shared_ptr<geometry_msgs::msg::PoseStamped>
      current_pose_ptr_; // !< @brief current measured pose
  std::shared_ptr<nav_msgs::msg::Odometry>
      measure_odom_ptr_; // !< @brief current measured pose
  std::shared_ptr<nav_msgs::msg::Odometry>
      system_odom_ptr_; // !< @brief current measured pose
  geometry_msgs::msg::PoseStamped
      current_ekf_pose_; // !< @brief current estimated pose
  geometry_msgs::msg::TwistStamped
      current_ekf_twist_; // !< @brief current estimated twist
  nav_msgs::msg::Odometry current_ekf_odom_;
  std::array<double, 36ul> current_pose_covariance_;
  std::array<double, 36ul> current_twist_covariance_;

  /**
   * @brief computes update & prediction of EKF for each ekf_dt_[s] time
   */
  void timerCallback();

  /**
   * @brief set odom measurement including pose and twist
   */
  void BESTPOSCallback(const novatel_oem7_msgs::msg::BESTPOS::SharedPtr msg);
  void TOPBESTPOSCallback(const novatel_oem7_msgs::msg::BESTPOS::SharedPtr msg);
  void
  BOTTOMINSPVACallback(const novatel_oem7_msgs::msg::INSPVA::SharedPtr msg);
  void TOPINSPVACallback(const novatel_oem7_msgs::msg::INSPVA::SharedPtr msg);
  
  void BOTTOMBESTVELCallback(const novatel_oem7_msgs::msg::BESTVEL::SharedPtr msg);

  void TOPBESTVELCallback(const novatel_oem7_msgs::msg::BESTVEL::SharedPtr msg);

  void HEADING2Callback(const novatel_oem7_msgs::msg::HEADING2::SharedPtr msg);
  void TOPHEADING2Callback(const novatel_oem7_msgs::msg::HEADING2::SharedPtr msg);

  void INSSTDEVCallback(const novatel_oem7_msgs::msg::INSSTDEV::SharedPtr msg);
  void
  TOPINSSTDEVCallback(const novatel_oem7_msgs::msg::INSSTDEV::SharedPtr msg);

  void MessageFilteringCallback(
      const sensor_msgs::msg::Imu ::ConstSharedPtr &imu_msg,
      const raptor_dbw_msgs::msg::WheelSpeedReport::ConstSharedPtr
          &wheel_speed_msg);
  void IMUCallback(const sensor_msgs::msg::Imu::SharedPtr msg);
  void TestCallback(const std_msgs::msg::Float64::SharedPtr msg);

  /**
   * @brief Check whether heading msgs have been updated recently.
   * 
   */
  void headingAgeCheck();

  /**
   * @brief initialization of EKF
   */
  void initEKF();

  /**
   * @brief compute EKF prediction
   */
  void predictKinematicsModel();

  /**
   * @brief compute EKF update with pose measurement
   * @param pose measurement value
   */
  void measurementUpdatePose(rclcpp::Time measurement_time_,
                             const double &corr_x_, const double &corr_y_,
                             const double &corr_yaw_);

  /**
   * @brief compute EKF update with pose measurement
   * @param twist measurement value
   */
  void measurementUpdateTwist(rclcpp::Time measurement_time_,
                              const double &pred_vel_x_,
                              const double &pred_yaw_rate_);

  /**
   * @brief check whether a measurement value falls within the mahalanobis
   * distance threshold
   * @param dist_max mahalanobis distance threshold
   * @param estimated current estimated state
   * @param measured measured state
   * @param estimated_cov current estimation covariance
   * @return whether it falls within the mahalanobis distance threshold
   */
  bool mahalanobisGate(const double &dist_max, const Eigen::MatrixXd &estimated,
                       const Eigen::MatrixXd &measured,
                       const Eigen::MatrixXd &estimated_cov);
  double GetmahalanobisDistance(const Eigen::MatrixXd &x,
                                const Eigen::MatrixXd &obj_x,
                                const Eigen::MatrixXd &cov);
  bool CalculateBestCorrection(const GPSCorrectionData_t &BottomDataIn,
                               const GPSCorrectionData_t &TopDataIn,
                               const VehPose_t &CurrentEstIn,
                               VehPose_t &BestCorrectionOut);
  bool GPSIgnoreGate(const double &dist_max,
                     const Eigen::MatrixXd &x,     // estimated
                     const Eigen::MatrixXd &obj_x, // correct
                     const Eigen::MatrixXd &cov);
  /**
   * @brief normalize yaw angle
   * @param yaw yaw angle
   * @return normalized yaw
   */
  double normalizeYaw(const double &yaw);

  /**
   * @brief set current EKF estimation result to current_ekf_pose_ &
   * current_ekf_twist_
   */
  void setCurrentResult();

  /**
   * @brief publish current EKF estimation result
   */
  void publishEstimateResult();

  /**
   * @brief for debug
   */
  void showCurrentX();

  friend class AWLocalizationNodeTestSuite; //  for test code
};

#endif // AW_EKF_LOCALIZER_NODE_H
