/**
 * @brief Pure Pursuit tracking on a trajectory
 **/
#ifndef BVS_CONTROL_UTILS_PURE_PURSUIT_TRACKER_H_
#define BVS_CONTROL_UTILS_PURE_PURSUIT_TRACKER_H_

#include "nav_msgs/msg/odometry.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"

namespace bvs_control {
namespace utils {

/**
 * @brief templated distance function for pure pursuit
 * @param point_a some point
 * @param point_b some point
 * @return distance between point_a and point_b
 **/
template<typename PointTA, typename PointTB>
double pursuit_dist(PointTA& point_a, PointTB& point_b);


template<> double
pursuit_dist<nav_msgs::msg::Odometry, nav_msgs::msg::Odometry>(
    nav_msgs::msg::Odometry& point_a,
    nav_msgs::msg::Odometry& point_b
);
template<> double
pursuit_dist<geometry_msgs::msg::PoseStamped, nav_msgs::msg::Odometry>(
    geometry_msgs::msg::PoseStamped& point_a,
    nav_msgs::msg::Odometry& point_b
);
template<> double
pursuit_dist<geometry_msgs::msg::PoseStamped, geometry_msgs::msg::PoseStamped>(
    geometry_msgs::msg::PoseStamped& point_a,
    geometry_msgs::msg::PoseStamped& point_b
);


/**
 * @brief templated pure pursuit function to find the earliest point
 *  in path pursuit_distance from position
 * @param[in] path some array of some point type
 * @param[in] position the position to run pure pursuit from
 * @param[in] pursuit_distance the distance to run pure pursuit with
 * @param[out] target_idx index to start the search from and also the result
 * @param[out] target_distance distance to the resulting point
 * @param[out] reached_end whether or not the algorithm reached the end of the trajectory
 *
 * @note the path must be starting somewhere around 
 **/
template<typename PathT, typename PointT>
void track(
    PathT& path,
    PointT& position,
    double pursuit_distance,
    unsigned int& target_idx,
    double& target_distance,
    bool& reached_end
) {
    reached_end = false;
    // Make sure we start within bounds
    if(target_idx < 0) {
        target_idx = 0;
    } else if(target_idx >= path.size()) {
        target_idx = path.size() - 1;
    }

    target_distance = pursuit_dist(path[target_idx], position);
    while(true) {
        // if the distance is less than the pursuit distance we try to advance
        if(target_distance < pursuit_distance) {
            if(target_idx + 1 >= path.size()) {
                reached_end = true;
            }
            // This will break if the trajectory wraps around 180 degrees
            // within the tracking horizon
            target_distance = pursuit_dist(path[target_idx + 1], position);
            ++target_idx;

        // Otherwise we try to go backwards on the trajectory
        // (which we might want to do if we slow down)
        } else {
            if(target_idx <= 0) {
                break;
            }
            auto dist_prev = pursuit_dist(path[target_idx - 1], position);
            // If the previous point is also greater than the pursuit distance
            // we will go back and select it
            if(dist_prev >= pursuit_distance) {
                --target_idx;
                target_distance = dist_prev;
            // Otherwise we are right where we want to be!
            } else {
                break;
            }
        }
    }
}

/**
 * @brief smooths a control signal over time
 * @param current_signal a signal at a given point in time
 * @param target_signal the signal we would like to eventually becom
 * @param delta_dt the allowed change in the signal over time
 * @param dt the change in time from the previous update
 * @return the smoothed signal
 **/
double smoothSignal(
    double current_signal,
    double target_signal, 
    double delta_dt,
    double dt
);


} /* namespace utils */
} /* namespace bvs_control */


#endif