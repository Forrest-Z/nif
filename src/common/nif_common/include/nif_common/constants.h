//  Copyright (c) 2021 Unmanned System Research Group @ KAIST
//  Author:

//
// Created by usrg on 6/26/21.
//

#ifndef NIF_COMMON_CONSTANTS_H
#define NIF_COMMON_CONSTANTS_H

#include <chrono>
#include <limits>
#include <rclcpp/qos.hpp>

namespace nif {
namespace common {
namespace constants {

/**
 * Maximum number of opponent on the track.
 */
const int NUMBER_OF_OPPO_MAX = 10;

/**
 * DEGREE to RADIAN.
 */
const double DEG2RAD = 0.0174533;

/**
 * RADIAN to DEGREE.
 */
const double RAD2DEG = 57.2958;

/**
 * Converstion factor from k/h to m/s.
 */
const double KPH2MS = 0.277778;

/**
 * Default period for general node. (50 Hz)
 */
const std::chrono::milliseconds PERIOD_DEFAULT(20);

/**
 * Default period for synchronized node.
 */
const std::chrono::microseconds SYNC_PERIOD_DEFAULT(10000);

/**
 * Min period for synchronized node.
 */
const std::chrono::microseconds SYNC_PERIOD_MIN(10000);

/**
 * Max period for synchronized node.
 */
const std::chrono::microseconds SYNC_PERIOD_MAX(10000);

/**
 * Default QoS parameter.
 * TODO look into details and take proper decision
 */
const rclcpp::QoS QOS_DEFAULT(5);

/**
 * Name for the main logger.
 */
const char* const LOG_MAIN_LOGGER_NAME = "MAIN_LOGGER";

namespace parameters
{
/**
 * Name for the GlobalParametersNode
 */
const std::string DELIMITER("/");

/**
 * Name for the GlobalParametersNode
 */
const std::string GLOBAL_PARAMETERS_NODE_NAME("global_parameters_node");

/**
 * Default global parameter server-connection timeout.
 */
const std::chrono::seconds GLOBAL_PARAMETERS_NODE_TIMEOUT(3);

namespace names {

/**
 * Name for the body frame id parameter.
 */
constexpr const char* FRAME_ID_BODY = "frames.body";

/**
 * Name for the global frame id parameter.
 */
constexpr const char* FRAME_ID_GLOBAL = "frames.global";


/**
 * Name for the ego odometry topic_name parameter.
 */
constexpr const char* TOPIC_ID_EGO_ODOMETRY = "topics.ego_odometry";

/**
 * Name for the system status topic_name parameter.
 */
constexpr const char* TOPIC_ID_SYSTEM_STATUS = "topics.system_status";

/**
 * Name for the race control status topic_name parameter.
 */
constexpr const char* TOPIC_ID_RACE_CONTROL_STATUS = "topics.race_control_status";

/**
 * Name for the ego power-train status topic_name parameter.
 */
constexpr const char* TOPIC_ID_EGO_POWERTRAIN_STATUS = "topics.ego_powertrain_status";


} // namespace names

/**
 * Default value for the body frame id parameter.
 */
 __attribute_deprecated__
constexpr const char* VALUE_BODY_FRAME_ID = "base_link";

/**
 * Default value for the global frame id parameter.
 */
 __attribute_deprecated__
 constexpr const char* VALUE_GLOBAL_FRAME_ID = "odom";

} // namespace parameters

namespace numeric {
/**
 * pi is pi
 */
const double PI = 3.141592653589793;

/**
 * Infinite value (float)
 */
const float INF = std::numeric_limits<float>::max();

/**
 * Epsilon value (float)
 */
const float EPSILON = std::numeric_limits<float>::epsilon();

} // namespace numeric

namespace planner {
/**
 * pi is pi
 */
const float WPT_MINIMUM_LEN = 1.0;

} // namespace planner

} // namespace constants
} // namespace common
} // namespace nif

#endif // NIF_COMMON_CONSTANTS_H
