#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "nif_common/constants.h"
#include "rclcpp/rclcpp.hpp"
#include "deep_orange_msgs/msg/ct_report.hpp"
#include "deep_orange_msgs/msg/pt_report.hpp"
#include "deep_orange_msgs/msg/misc_report.hpp"
#include "std_msgs/msg/float64.hpp"
#include "nif_msgs/msg/system_status.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"

using namespace std::chrono_literals;

class Telemetry : public rclcpp::Node
{
  public:
    Telemetry()
    : Node("telemetry")
    {
      // setup QOS to be best effort
      auto qos = rclcpp::QoS(rclcpp::QoSInitialization(RMW_QOS_POLICY_HISTORY_KEEP_LAST, 1));
      qos.best_effort();
      pub_ct_report = this->create_publisher<deep_orange_msgs::msg::CtReport>("/telemetry/ct_report", qos);
      pub_pt_report = this->create_publisher<deep_orange_msgs::msg::PtReport>("/telemetry/pt_report", qos);
      pub_misc_report_do = this->create_publisher<deep_orange_msgs::msg::MiscReport>("/telemetry/misc_report_do", qos);
      pub_system_status = this->create_publisher<nif_msgs::msg::SystemStatus>("/telemetry/system_status", qos);
      pub_ego_odometry = this->create_publisher<geometry_msgs::msg::PoseStamped>("/telemetry/odom", qos);

      sub_ct_report = this->create_subscription<deep_orange_msgs::msg::CtReport>(
        "/raptor_dbw_interface/ct_report", nif::common::constants::QOS_SENSOR_DATA, std::bind(&Telemetry::ct_report_callback, this, std::placeholders::_1));
      sub_pt_report = this->create_subscription<deep_orange_msgs::msg::PtReport>(
        "/raptor_dbw_interface/pt_report", nif::common::constants::QOS_SENSOR_DATA, std::bind(&Telemetry::pt_report_callback, this, std::placeholders::_1));
      sub_misc_report_do = this->create_subscription<deep_orange_msgs::msg::MiscReport>(
        "/raptor_dbw_interface/misc_report_do", nif::common::constants::QOS_SENSOR_DATA, std::bind(&Telemetry::misc_report_do_callback, this, std::placeholders::_1));
      sub_system_status = this->create_subscription<nif_msgs::msg::SystemStatus>(
        "/system/status", nif::common::constants::QOS_SENSOR_DATA, std::bind(&Telemetry::system_status_callback, this, std::placeholders::_1));
      sub_ego_odometry = this->create_subscription<nav_msgs::msg::Odometry>(
        "/aw_localization/ekf/odom", nif::common::constants::QOS_SENSOR_DATA, std::bind(&Telemetry::ego_odometry_callback, this, std::placeholders::_1));

      timer_ = this->create_wall_timer(
        250ms, std::bind(&Telemetry::timer_callback, this));
    }

  private:
    void timer_callback()
    {
      pub_ct_report->publish(msg_ct_report);
      pub_pt_report->publish(msg_pt_report);
      pub_misc_report_do->publish(msg_misc_report_do);
      pub_system_status->publish(msg_system_status);
      pub_ego_odometry->publish(msg_ego_odometry);
    }
    void ct_report_callback(const deep_orange_msgs::msg::CtReport::SharedPtr msg)
    {
        msg_ct_report = *msg;
    }
    void pt_report_callback(const deep_orange_msgs::msg::PtReport::SharedPtr msg)
    {
        msg_pt_report = *msg;
    }
    void misc_report_do_callback(const deep_orange_msgs::msg::MiscReport::SharedPtr msg)
    {
        msg_misc_report_do = *msg;
    }
    void system_status_callback(const nif_msgs::msg::SystemStatus::SharedPtr msg)
    {
        msg_system_status = *msg;
    }
    void lateral_error_callback(const std_msgs::msg::Float64::SharedPtr msg)
    {
        msg_lateral_error = *msg;
    }
    void lookahead_error_callback(const std_msgs::msg::Float64::SharedPtr msg)
    {
        msg_lookahead_error = *msg;
    }    
    void ego_odometry_callback(const nav_msgs::msg::Odometry::SharedPtr msg)
    {
        msg_ego_odometry.header = std::move(msg->header);
        msg_ego_odometry.pose = std::move(msg->pose.pose);
    }
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<deep_orange_msgs::msg::CtReport>::SharedPtr pub_ct_report;
    rclcpp::Publisher<deep_orange_msgs::msg::PtReport>::SharedPtr pub_pt_report;
    rclcpp::Publisher<deep_orange_msgs::msg::MiscReport>::SharedPtr pub_misc_report_do;
    rclcpp::Publisher<nif_msgs::msg::SystemStatus>::SharedPtr pub_system_status;
    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pub_ego_odometry;

    rclcpp::Subscription<deep_orange_msgs::msg::CtReport>::SharedPtr sub_ct_report;
    rclcpp::Subscription<deep_orange_msgs::msg::PtReport>::SharedPtr sub_pt_report;
    rclcpp::Subscription<deep_orange_msgs::msg::MiscReport>::SharedPtr sub_misc_report_do;
    rclcpp::Subscription<nif_msgs::msg::SystemStatus>::SharedPtr sub_system_status;
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr sub_ego_odometry;

    deep_orange_msgs::msg::CtReport msg_ct_report;
    deep_orange_msgs::msg::PtReport msg_pt_report;
    deep_orange_msgs::msg::MiscReport msg_misc_report_do;
    nif_msgs::msg::SystemStatus msg_system_status;
    std_msgs::msg::Float64 msg_lookahead_error;
    std_msgs::msg::Float64 msg_lateral_error;
    geometry_msgs::msg::PoseStamped msg_ego_odometry;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Telemetry>());
  rclcpp::shutdown();
  return 0;
}
