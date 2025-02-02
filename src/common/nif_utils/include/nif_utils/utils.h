//  Copyright (c) 2021 Unmanned System Research Group @ KAIST
//  Author:

//
// Created by usrg on 6/22/21.
//

#ifndef NIF_UTILS_H
#define NIF_UTILS_H

#include "geometry_msgs/msg/point.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/quaternion.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "nav_msgs/msg/path.hpp"
#include "nif_common/constants.h"
#include "rclcpp/rclcpp.hpp"
#include "tf2/LinearMath/Transform.h"
#include "tf2/convert.h"
#include <string>

namespace nif {
namespace common {
namespace utils {

/**
 * @brief Return the index of the first element greater-equal value in the
 * array, or the index of the last element if none is greater-equal value.
 *
 * @param vec
 * @param value
 * @return int
 */
inline int closestIndex(std::vector<float> const &vec, double value) {
  auto it = std::lower_bound(vec.begin(), vec.end(), value);
  if (it == vec.end()) {
    return distance(vec.begin(), --it);
  }
  
  return distance(vec.begin(), it);
};


namespace time {
/**
 * Time in seconds
 **/
double secs(rclcpp::Time t);

/**
 * Duration in seconds
 **/
double secs(rclcpp::Duration t);
} // namespace time

namespace geometry {

const float MPH_TO_KPH_FACTOR = 1.60934;
const float KPH_TO_MPH_FACTOR = 0.621371;

/**
 * Calculate Euclidean distance between Point/Pose/PoseStamped
 * @param a Point/Pose/PoseStamped
 * @param b Point/Pose/PoseStamped
 * @return Euclidean distance between Point/Pose/PoseStamped
 */
double calEuclideanDistance(const geometry_msgs::msg::PoseStamped &a,
                            const geometry_msgs::msg::PoseStamped &b);

/**
 * Calculate Euclidean distance between Point/Pose/PoseStamped
 * @param a Point/Pose/PoseStamped
 * @param b Point/Pose/PoseStamped
 * @return Euclidean distance between Point/Pose/PoseStamped
 */
double calEuclideanDistance(const geometry_msgs::msg::Pose &a,
                            const geometry_msgs::msg::Pose &b);

/**
 * Calculate Euclidean distance between Point/Pose/PoseStamped
 * @param a Point/Pose/PoseStamped
 * @param b Point/Pose/PoseStamped
 * @return Euclidean distance between Point/Pose/PoseStamped
 */
double calEuclideanDistance(const geometry_msgs::msg::Point &a,
                            const geometry_msgs::msg::Point &b);


/**
 * Calculate Euclidean distance between Point/Pose/PoseStamped
 * @param a_x (double) point a x coordinate
 * @param a_y (double) point a y coordinate
 * @param a_z (double) point a z coordinate
 * @param b_x (double) point b x coordinate
 * @param b_y (double) point b y coordinate
 * @param b_z (double) point b z coordinate
 * @return Euclidean distance between Point/Pose/PoseStamped
 */
double calEuclideanDistance(double a_x, double a_y, double a_z,
                            double b_x, double b_y, double b_z);

//    TODO: fix static override problems
//    double calEuclideanDistance(const std::vector<double,double>& a, const
//    std::vector<double,double>& b);

/**
 * Calculate squared Euclidean distance between two PoseStamped
 * @param a Point/Pose/PoseStamped
 * @param b Point/Pose/PoseStamped
 * @return Euclidean distance between Point/Pose/PoseStamped
 */
double calEuclideanDistanceSquared(const geometry_msgs::msg::PoseStamped& a,
                            const geometry_msgs::msg::PoseStamped& b);

/**
 * Calculate squared Euclidean distance between two Pose
 * @param a Point/Pose/PoseStamped
 * @param b Point/Pose/PoseStamped
 * @return Euclidean distance between Point/Pose/PoseStamped
 */
double calEuclideanDistanceSquared(const geometry_msgs::msg::Pose& a,
                            const geometry_msgs::msg::Pose& b);

/**
 * Calculate squared Euclidean distance between two Point
 * @param a Point/Pose/PoseStamped
 * @param b Point/Pose/PoseStamped
 * @return Euclidean distance between Point/Pose/PoseStamped
 */
double calEuclideanDistanceSquared(const geometry_msgs::msg::Point& a,
                            const geometry_msgs::msg::Point& b);

//    TODO: fix static override problems
//    double calEuclideanDistance(const std::vector<double,double>& a, const
//    std::vector<double,double>& b);


/**
 * Convert linear velocity from miles-per-hour to kilometers-per-hour
 * @param mph Linear velocity in miles-per-hour
 * @return Linear velocity in kilometers-per-hour
 */
double mph2kph(double mph);

/**
 * Convert linear velocity from kilometers-per-hour to miles-per-hour
 * @param mph Linear velocity in kilometers-per-hour
 * @return Linear velocity in miles-per-hour
 */
double kph2mph(double kph);

/**
 * Convert linear velocity from mile-per-hour to miles-per-sec
 * @param mph Linear velocity in mile-per-hour
 * @return Linear velocity in miles-per-sec
 */
double mph2mps(double mph);

} // namespace geometry

namespace io {
//  TODO: define precisely (do we need this?)
std::vector<rclcpp::Parameter> &readConfig(std::string &file_name);

} // namespace io

// TODO: numeric is an awful name, needs something better.
namespace numeric {

//    TODO: provide exaustive description of these two functions, along wth teir
//    params' description.
//     std::tuple<min_value, min_value_idx>
//    findMinValueNIdx(std::vector<double>& vec); std::tuple<max_value,
//    max_value_idx> findMaxValueNIdx(std::vector<double>& vec);

/**
 * Clip the target value to min-max bounds.
 * @tparam T datatype must implement comparison operators
 * @param min Minimum value.
 * @param max Maximum value.
 * @param target Target value to be clipped.
 * @return Clipped value.
 */
template <typename T>
constexpr inline const T &clip(const T &min, const T &max, const T &target) {
  return std::max<T>(min, std::min<T>(target, max));
}

} // namespace numeric

namespace coordination {

inline double quat2yaw(const geometry_msgs::msg::Quaternion &data) {
  return atan2(2 * (data.w * data.z + data.x * data.y),
               1 - 2 * (data.y * data.y + data.z * data.z));
};

geometry_msgs::msg::Quaternion euler2quat(double yaw, double pitch,
                                          double roll);

inline double angle_wrap(double diff);

geometry_msgs::msg::PoseStamped
getPtBodytoGlobal(const nav_msgs::msg::Odometry &current_odom_,
                  const geometry_msgs::msg::PoseStamped &point_in_body_);

geometry_msgs::msg::PoseStamped
getPtBodytoGlobal(const nav_msgs::msg::Odometry &current_odom_,
                  const geometry_msgs::msg::Pose &point_in_body_);

geometry_msgs::msg::Pose
getPtBodytoGlobal(const geometry_msgs::msg::Pose &current_pose_,
                  const geometry_msgs::msg::Pose &point_in_body_);

geometry_msgs::msg::PoseStamped
getPtStampedBodytoGlobal(
  const geometry_msgs::msg::Pose& current_pose_,
  const geometry_msgs::msg::Pose& point_in_body_);

geometry_msgs::msg::PoseStamped
getPtGlobaltoBody(const nav_msgs::msg::Odometry& current_odom_,
                  const geometry_msgs::msg::PoseStamped& point_in_global_);

geometry_msgs::msg::PoseStamped
getPtGlobaltoBody(const nav_msgs::msg::Odometry &current_odom_,
                  const geometry_msgs::msg::Pose &point_in_global_);

geometry_msgs::msg::PoseStamped
getPtGlobaltoBody(const nav_msgs::msg::Odometry &current_odom_,
                  const double &global_x_, const double &global_y_);

geometry_msgs::msg::Pose
getPtGlobaltoBody(const geometry_msgs::msg::Pose &current_pose_,
                  const geometry_msgs::msg::Pose &point_in_global_);

geometry_msgs::msg::PoseStamped
getPtStampedGlobaltoBody(
  const geometry_msgs::msg::Pose& current_pose_,
  const geometry_msgs::msg::Pose& point_in_global_);

nav_msgs::msg::Path getPathGlobaltoBody(const nav_msgs::msg::Odometry& current_odom_,
                                        const nav_msgs::msg::Path& path_in_global_);

} // namespace coordination

namespace naming {

inline const std::string getGlobalParamName(const std::string &param_name) {
  return nif::common::constants::parameters::DELIMITER +
         nif::common::constants::parameters::GLOBAL_PARAMETERS_NODE_NAME +
         nif::common::constants::parameters::DELIMITER + param_name;
}

} // namespace naming

namespace algebra {
  /**
   * Calculate cross-product
   * @param vector_a Point/Pose/PoseStamped
   * @param vector_b Point/Pose/PoseStamped
   * @return Cross-product vector_c = vector_a x vector_b
   */
  geometry_msgs::msg::Point calCrossProduct(
    const geometry_msgs::msg::Point& vector_a,
    const geometry_msgs::msg::Point& vector_b);

} // namespace algebra


} // namespace utils
} // namespace common
} // namespace nif

#endif // NIF_UTILS_H