/*
 * Copyright 2018-2019 Autoware Foundation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nif_objects_tracker_nodes/imm/imm_ukf_pda.h"

ImmUkfPda::ImmUkfPda(const std::string &config_file_path_)
    : target_id_(0), // assign unique ukf_id_ to each tracking targets
      init_(false), frame_count_(0) {

  YAML::Node config = YAML::LoadFile(config_file_path_);

  if (!config["gating_thres"]) {
    throw std::runtime_error("gating_thres is not properly set.");
  } else {
    gating_thres_ = config["gating_thres"].as<double>();
  }

  if (!config["gate_probability"]) {
    throw std::runtime_error("gate_probability is not properly set.");
  } else {
    gate_probability_ = config["gate_probability"].as<double>();
  }

  if (!config["detection_probability"]) {
    throw std::runtime_error("detection_probability is not properly set.");
  } else {
    detection_probability_ = config["detection_probability"].as<double>();
  }

  if (!config["life_time_thres"]) {
    throw std::runtime_error("life_time_thres is not properly set.");
  } else {
    life_time_thres_ = config["life_time_thres"].as<int>();
  }

  if (!config["merge_distance_threshold"]) {
    throw std::runtime_error("merge_distance_threshold is not properly set.");
  } else {
    merge_distance_threshold_ = config["merge_distance_threshold"].as<double>();
  }

  if (!config["static_velocity_thres"]) {
    throw std::runtime_error("static_velocity_thres is not properly set.");
  } else {
    static_velocity_thres_ = config["static_velocity_thres"].as<double>();
  }

  if (!config["static_num_history_thres"]) {
    throw std::runtime_error(
        "static_num_history_thres is not properly set.");
  } else {
    static_num_history_thres_ = config["static_num_history_thres"].as<int>();
  }

  if (!config["prevent_explosion_thres"]) {
    throw std::runtime_error("prevent_explosion_thres is not properly set.");
  } else {
    prevent_explosion_thres_ = config["prevent_explosion_thres"].as<int>();
  }

  if (!config["use_sukf"]) {
    throw std::runtime_error("use_sukf is not properly set.");
  } else {
    use_sukf_ = config["use_sukf"].as<bool>();
  }

  if (!config["tracking_frame"]) {
    throw std::runtime_error("tracking_frame is not properly set.");
  } else {
    tracking_frame_ = config["tracking_frame"].as<std::string>();
  }
}

void ImmUkfPda::setDetectionResult(
    const nif_msgs::msg::DetectedObjectArray &input) {
  previous_loop_det_ = input;

  nif_msgs::msg::DetectedObjectArray transformed_input;
  nif_msgs::msg::DetectedObjectArray detected_objects_output_global;
  nif_msgs::msg::DetectedObjectArray detected_objects_output_local;

  transformPoseToGlobal(input, transformed_input);
  tracker(transformed_input, detected_objects_output_global);
  transformPoseToLocal(detected_objects_output_global, detected_objects_output_local);

  tracked_object_ = detected_objects_output_local;
}

// nif_msgs::msg::DetectedObjectArray ImmUkfPda::getTrackedResult()
// {
//     return tracked_object_;
// }

nif_msgs::msg::Perception3DArray
ImmUkfPda::getTrackedResult(nif_msgs::msg::DetectedObjectArray &input) {
  setDetectionResult(input);

  // NOTE : Convert from detectionObject to PerceptionObject
  // TODO : Assign velocities

  nif_msgs::msg::Perception3DArray out_tracked_objects;

  out_tracked_objects.header = tracked_object_.header;
  out_tracked_objects.perception_list.clear();

  for (int det_idx = 0; det_idx < tracked_object_.objects.size(); det_idx++) {
    nif_msgs::msg::Perception3D p3d;
    p3d.detection_result_3d.center.position.x =
        tracked_object_.objects[det_idx].pose.position.x;
    p3d.detection_result_3d.center.position.y =
        tracked_object_.objects[det_idx].pose.position.y;
    p3d.detection_result_3d.center.position.z =
        tracked_object_.objects[det_idx].pose.position.z;

    p3d.detection_result_3d.center.orientation.x = tracked_object_.objects[det_idx].pose.orientation.x;
    p3d.detection_result_3d.center.orientation.y = tracked_object_.objects[det_idx].pose.orientation.y;
    p3d.detection_result_3d.center.orientation.z = tracked_object_.objects[det_idx].pose.orientation.z;
    p3d.detection_result_3d.center.orientation.w = tracked_object_.objects[det_idx].pose.orientation.w;

    p3d.obj_velocity_in_global = tracked_object_.objects[det_idx].velocity;
    // TODO: assign local vel
    out_tracked_objects.perception_list.push_back(p3d);
  }

  return out_tracked_objects;
}

void ImmUkfPda::setEgoOdom(const nav_msgs::msg::Odometry &odom_) {
  ego_odom_ = odom_;
}

void ImmUkfPda::transformPoseToGlobal(
    const nif_msgs::msg::DetectedObjectArray &input,
    nif_msgs::msg::DetectedObjectArray &transformed_input) {
  
  input_header_ = input.header;
  transformed_input.header = input_header_;

  for (auto const &object : input.objects) {
    geometry_msgs::msg::Pose out_pose = 
      nif::common::utils::coordination::getPtBodytoGlobal(
        ego_odom_,
        object.pose
    ).pose;

    nif_msgs::msg::DetectedObject dd;
    dd.header = input.header;
    dd = object;
    dd.pose = out_pose;

    transformed_input.objects.push_back(dd);
  }
}

void ImmUkfPda::transformPoseToLocal(
    const nif_msgs::msg::DetectedObjectArray &input,
    nif_msgs::msg::DetectedObjectArray &transformed_input) {
  
  input_header_ = input.header;
  transformed_input.header = input_header_;

  for (auto const &object : input.objects) {
    geometry_msgs::msg::Pose out_pose = 
      nif::common::utils::coordination::getPtGlobaltoBody(
        ego_odom_,
        object.pose).pose;

    nif_msgs::msg::DetectedObject dd;
    dd.header = input.header;
    dd = object;
    dd.pose = out_pose;

    transformed_input.objects.push_back(dd);
  }
}

void ImmUkfPda::transformPoseToLocal(
    nif_msgs::msg::DetectedObjectArray &detected_objects_output) {
  detected_objects_output.header = input_header_;

  for (auto &object : detected_objects_output.objects) {
    geometry_msgs::msg::Pose out_pose = 
      nif::common::utils::coordination::getPtGlobaltoBody(
        ego_odom_,
        object.pose).pose;
    object.header = input_header_;
    object.pose = out_pose;
  }
}

void ImmUkfPda::measurementValidation(
    const nif_msgs::msg::DetectedObjectArray &input, UKF &target,
    const bool second_init, const Eigen::VectorXd &max_det_z,
    const Eigen::MatrixXd &max_det_s,
    std::vector<nif_msgs::msg::DetectedObject> &object_vec,
    std::vector<bool> &matching_vec) {
  // alert: different from original imm-pda filter, here picking up most
  // likely measurement if making it allows to have more than one measurement,
  // you will see non semipositive definite covariance
  bool exists_smallest_nis_object = false;
  double smallest_nis = std::numeric_limits<double>::max();
  int smallest_nis_ind = 0;
  for (size_t i = 0; i < input.objects.size(); i++) {
    double x = input.objects[i].pose.position.x;
    double y = input.objects[i].pose.position.y;

    Eigen::VectorXd meas = Eigen::VectorXd(2);
    meas << x, y;

    Eigen::VectorXd diff = meas - max_det_z;
    double nis = diff.transpose() * max_det_s.inverse() * diff;

    // std::cout << "nis: " << nis << std::endl; // @DEBUG

    if (nis < gating_thres_) {
      if (nis < smallest_nis) {
        smallest_nis = nis;
        target.object_ = input.objects[i];
        smallest_nis_ind = i;
        exists_smallest_nis_object = true;
      }
    }
  }
  if (exists_smallest_nis_object) {
    // std::cout << "exists_smallest_nis_object: " << exists_smallest_nis_object << std::endl; // @DEBUG

    matching_vec[smallest_nis_ind] = true;
    // if (use_vectormap_ && has_subscribed_vectormap_)
    // {
    //     nif_msgs::msg::DetectedObject direction_updated_object;
    //     bool use_direction_meas = updateDirection(
    //         smallest_nis, target.object_, direction_updated_object, target);
    //     if (use_direction_meas)
    //     {
    //         object_vec.push_back(direction_updated_object);
    //     }
    //     else
    //     {
    //         object_vec.push_back(target.object_);
    //     }
    // }
    // else
    // {
    //     object_vec.push_back(target.object_);
    // }
    object_vec.push_back(target.object_);
  }
}

// bool ImmUkfPda::updateDirection(const double smallest_nis,
//                                 const nif_msgs::msg::DetectedObject
//                                 &in_object, nif_msgs::msg::DetectedObject
//                                 &out_object, UKF &target)
// {
//     bool use_lane_direction = false;
//     target.is_direction_cv_available_ = false;
//     target.is_direction_ctrv_available_ = false;
//     bool get_lane_success =
//         storeObjectWithNearestLaneDirection(in_object, out_object);
//     if (!get_lane_success)
//     {
//         return use_lane_direction;
//     }
//     target.checkLaneDirectionAvailability(out_object,
//     lane_direction_chi_thres_,
//                                           use_sukf_);
//     if (target.is_direction_cv_available_ ||
//         target.is_direction_ctrv_available_)
//     {
//         use_lane_direction = true;
//     }
//     return use_lane_direction;
// }

void ImmUkfPda::updateTargetWithAssociatedObject(
    const std::vector<nif_msgs::msg::DetectedObject> &object_vec, UKF &target) {
  target.lifetime_++;
  if (!target.object_.label.empty() && target.object_.label != "unknown") {
    target.label_ = target.object_.label;
  }
  updateTrackingNum(object_vec, target);
  if (target.tracking_num_ == TrackingState::Stable ||
      target.tracking_num_ == TrackingState::Occlusion) {
    target.is_stable_ = true;
  }
}

void ImmUkfPda::updateBehaviorState(const UKF &target,
                                    nif_msgs::msg::DetectedObject &object) {
  if (target.mode_prob_cv_ > target.mode_prob_ctrv_ &&
      target.mode_prob_cv_ > target.mode_prob_rm_) {
    object.behavior_state = MotionModel::CV;
  } else if (target.mode_prob_ctrv_ > target.mode_prob_cv_ &&
             target.mode_prob_ctrv_ > target.mode_prob_rm_) {
    object.behavior_state = MotionModel::CTRV;
  } else {
    object.behavior_state = MotionModel::RM;
  }
}

void ImmUkfPda::initTracker(const nif_msgs::msg::DetectedObjectArray &input,
                            const rclcpp::Time timestamp) {
  
  timestamp_ = timestamp;

  // for (size_t i = 0; i < input.objects.size(); i++) {
  //   double px = input.objects[i].pose.position.x;
  //   double py = input.objects[i].pose.position.y;
  //   Eigen::VectorXd init_meas = Eigen::VectorXd(2);
  //   init_meas << px, py;

  //   UKF ukf;
  //   ukf.initialize(init_meas, timestamp, target_id_);
  //   targets_.push_back(ukf);
  //   target_id_++;
  // }

  // @DEBUG: Single Track
  if (!input.objects.empty())
  {
    double px = 0.0;
    double py = 0.0;
    for (size_t i = 0; i < input.objects.size(); i++) {
      px += input.objects[i].pose.position.x;
      py += input.objects[i].pose.position.y;
    }
    px = px / input.objects.size();
    py = py / input.objects.size();
    // double px = input.objects[0].pose.position.x;
    // double py = input.objects[0].pose.position.y;
    Eigen::VectorXd init_meas = Eigen::VectorXd(2);
    init_meas << px, py;

    UKF ukf;
    ukf.initialize(init_meas, timestamp, target_id_);
    targets_.push_back(ukf);

    // std::cout << "Track re-initialized. px: " << px << "; py: " << py << std::endl; // @DEBUG

    init_ = true;
  }
}

void ImmUkfPda::secondInit(
    UKF &target, 
    const std::vector<nif_msgs::msg::DetectedObject> &object_vec,
    const rclcpp::Duration dt) {
  if (object_vec.size() == 0) {
    // std::cout << "Second init end in Target Die." << std::endl; // @DEBUG
    target.tracking_num_ = TrackingState::Die;
    return;
  }
  // record init measurement for env classification
  target.init_meas_ << target.x_merge_(0), target.x_merge_(1);

  double dt_s = dt.seconds();

  // state update
  double target_x = object_vec[0].pose.position.x;
  double target_y = object_vec[0].pose.position.y;
  double target_diff_x = target_x - target.x_merge_(0);
  double target_diff_y = target_y - target.x_merge_(1);
  double target_yaw = atan2(target_diff_y, target_diff_x);
  double dist =
      sqrt(target_diff_x * target_diff_x + target_diff_y * target_diff_y);
  double target_v = dist / dt_s;

    // std::cout << "secondInit: target_x: " << target_x << "; target_y: " << target_y << std::endl; // @DEBUG

  while (target_yaw > M_PI)
    target_yaw -= 2. * M_PI;
  while (target_yaw < -M_PI)
    target_yaw += 2. * M_PI;

  target.x_merge_(0) = target.x_cv_(0) = target.x_ctrv_(0) = target.x_rm_(0) =
      target_x;
  target.x_merge_(1) = target.x_cv_(1) = target.x_ctrv_(1) = target.x_rm_(1) =
      target_y;
  target.x_merge_(2) = target.x_cv_(2) = target.x_ctrv_(2) = target.x_rm_(2) =
      target_v;
  target.x_merge_(3) = target.x_cv_(3) = target.x_ctrv_(3) = target.x_rm_(3) =
      target_yaw;

  target.tracking_num_++;
  return;
}

void ImmUkfPda::updateTrackingNum(
    const std::vector<nif_msgs::msg::DetectedObject> &object_vec, UKF &target) {
  if (object_vec.size() > 0) {
    // std::cout << "tracking_num_: " << target.tracking_num_ << std::endl; // @DEBUG

    if (target.tracking_num_ < TrackingState::Stable) {
      target.tracking_num_++;
    } else if (target.tracking_num_ == TrackingState::Stable) {
      target.tracking_num_ = TrackingState::Stable;
    } else if (target.tracking_num_ >= TrackingState::Stable &&
               target.tracking_num_ < TrackingState::Lost) {
      target.tracking_num_ = TrackingState::Stable;
    } else if (target.tracking_num_ == TrackingState::Lost) {
      target.tracking_num_ = TrackingState::Die;
    }
  } else {
    // std::cout << "No object associated with target" << std::endl; // @DEBUG

    // @DEBUG no need to remove the target track? Remove it to be able to re-init
    if (target.tracking_num_ < TrackingState::Stable) {
      target.tracking_num_ = TrackingState::Die;
    } else if (target.tracking_num_ >= TrackingState::Stable &&
               target.tracking_num_ < TrackingState::Lost) {
      target.tracking_num_++;
    } else if (target.tracking_num_ == TrackingState::Lost) {
      target.tracking_num_ = TrackingState::Die;
    }
  }

  return;
}

bool ImmUkfPda::probabilisticDataAssociation(
    const nif_msgs::msg::DetectedObjectArray &input, const rclcpp::Duration dt,
    std::vector<bool> &matching_vec,
    std::vector<nif_msgs::msg::DetectedObject> &object_vec, UKF &target) {
  double det_s = 0;
  Eigen::VectorXd max_det_z;
  Eigen::MatrixXd max_det_s;
  bool success = true;

  if (use_sukf_) {
    max_det_z = target.z_pred_ctrv_;
    max_det_s = target.s_ctrv_;
    det_s = max_det_s.determinant();
  } else {
    // find maxDetS associated with predZ
    target.findMaxZandS(max_det_z, max_det_s);
    det_s = max_det_s.determinant();
  }

  // prevent ukf not to explode
  if (std::isnan(det_s) || det_s > prevent_explosion_thres_) {
    // std::cout << "Prevent explosion. TrackingState -> Die." << std::endl; // @DEBUG
    target.tracking_num_ = TrackingState::Die;
    success = false;
    return success;
  }

  bool is_second_init;
  if (target.tracking_num_ == TrackingState::Init) {
    is_second_init = true;
  } else {
    is_second_init = false;
  }

  // measurement gating
  measurementValidation(input, target, is_second_init, max_det_z, max_det_s,
                        object_vec, matching_vec);

  // second detection for a target: update v and yaw
  if (is_second_init) {
    secondInit(target, object_vec, dt);
    success = false;
    return success;
  }

  updateTargetWithAssociatedObject(object_vec, target);

  if (target.tracking_num_ == TrackingState::Die) {
    success = false;
    return success;
  }
  return success;
}

void ImmUkfPda::makeNewTargets(const rclcpp::Time timestamp,
                               const nif_msgs::msg::DetectedObjectArray &input,
                               const std::vector<bool> &matching_vec) {
  for (size_t i = 0; i < input.objects.size(); i++) {
    if (matching_vec[i] == false) {
      double px = input.objects[i].pose.position.x;
      double py = input.objects[i].pose.position.y;
      Eigen::VectorXd init_meas = Eigen::VectorXd(2);
      init_meas << px, py;

      UKF ukf;
      ukf.initialize(init_meas, timestamp, target_id_);
      ukf.object_ = input.objects[i];
      targets_.push_back(ukf);
      target_id_++;
    }
  }
}

void ImmUkfPda::staticClassification() {
  for (size_t i = 0; i < targets_.size(); i++) {
    // targets_[i].x_merge_(2) is referred for estimated velocity
    double current_velocity = std::abs(targets_[i].x_merge_(2));
    targets_[i].vel_history_.push_back(current_velocity);
    if (targets_[i].tracking_num_ == TrackingState::Stable &&
        targets_[i].lifetime_ > life_time_thres_) {
      int index = 0;
      double sum_vel = 0;
      double avg_vel = 0;
      for (auto rit = targets_[i].vel_history_.rbegin();
           index < static_num_history_thres_; ++rit) {
        index++;
        sum_vel += *rit;
      }
      avg_vel = double(sum_vel / static_num_history_thres_);

      if (avg_vel < static_velocity_thres_ &&
          current_velocity < static_velocity_thres_) {
        targets_[i].is_static_ = true;
      }
    }
  }
}

bool ImmUkfPda::arePointsClose(const geometry_msgs::msg::Point &in_point_a,
                               const geometry_msgs::msg::Point &in_point_b,
                               float in_radius) {
  return (fabs(in_point_a.x - in_point_b.x) <= in_radius) &&
         (fabs(in_point_a.y - in_point_b.y) <= in_radius);
}

bool ImmUkfPda::arePointsEqual(const geometry_msgs::msg::Point &in_point_a,
                               const geometry_msgs::msg::Point &in_point_b) {
  return arePointsClose(in_point_a, in_point_b, CENTROID_DISTANCE);
}

bool ImmUkfPda::isPointInPool(
    const std::vector<geometry_msgs::msg::Point> &in_pool,
    const geometry_msgs::msg::Point &in_point) {
  for (size_t j = 0; j < in_pool.size(); j++) {
    if (arePointsEqual(in_pool[j], in_point)) {
      return true;
    }
  }
  return false;
}

nif_msgs::msg::DetectedObjectArray ImmUkfPda::removeRedundantObjects(
    const nif_msgs::msg::DetectedObjectArray &in_detected_objects,
    const std::vector<size_t> in_tracker_indices) {
  if (in_detected_objects.objects.size() != in_tracker_indices.size())
    return in_detected_objects;

  nif_msgs::msg::DetectedObjectArray resulting_objects;
  resulting_objects.header = in_detected_objects.header;

  std::vector<geometry_msgs::msg::Point> centroids;
  // create unique points
  for (size_t i = 0; i < in_detected_objects.objects.size(); i++) {
    if (!isPointInPool(centroids,
                       in_detected_objects.objects[i].pose.position)) {
      centroids.push_back(in_detected_objects.objects[i].pose.position);
    }
  }
  // assign objects to the points
  std::vector<std::vector<size_t>> matching_objects(centroids.size());
  for (size_t k = 0; k < in_detected_objects.objects.size(); k++) {
    const auto &object = in_detected_objects.objects[k];
    for (size_t i = 0; i < centroids.size(); i++) {
      if (arePointsClose(object.pose.position, centroids[i],
                         merge_distance_threshold_)) {
        matching_objects[i].push_back(
            k); // store index of matched object to this point
      }
    }
  }
  // get oldest object on each point
  for (size_t i = 0; i < matching_objects.size(); i++) {
    size_t oldest_object_index = 0;
    int oldest_lifespan = -1;
    std::string best_label;
    for (size_t j = 0; j < matching_objects[i].size(); j++) {
      size_t current_index = matching_objects[i][j];
      int current_lifespan =
          targets_[in_tracker_indices[current_index]].lifetime_;
      if (current_lifespan > oldest_lifespan) {
        oldest_lifespan = current_lifespan;
        oldest_object_index = current_index;
      }
      if (!targets_[in_tracker_indices[current_index]].label_.empty() &&
          targets_[in_tracker_indices[current_index]].label_ != "unknown") {
        best_label = targets_[in_tracker_indices[current_index]].label_;
      }
    }
    // delete nearby targets except for the oldest target
    for (size_t j = 0; j < matching_objects[i].size(); j++) {
      size_t current_index = matching_objects[i][j];
      if (current_index != oldest_object_index) {
        targets_[in_tracker_indices[current_index]].tracking_num_ =
            TrackingState::Die;
      }
    }
    nif_msgs::msg::DetectedObject best_object;
    best_object = in_detected_objects.objects[oldest_object_index];
    if (best_label != "unknown" && !best_label.empty()) {
      best_object.label = best_label;
    }

    resulting_objects.objects.push_back(best_object);
  }

  return resulting_objects;
}

void ImmUkfPda::makeOutput(
    const nif_msgs::msg::DetectedObjectArray &input,
    const std::vector<bool> &matching_vec,
    nif_msgs::msg::DetectedObjectArray &detected_objects_output) {
  nif_msgs::msg::DetectedObjectArray tmp_objects;
  tmp_objects.header = input.header;
  std::vector<size_t> used_targets_indices;
  for (size_t i = 0; i < targets_.size(); i++) {

    double tx = targets_[i].x_merge_(0);
    double ty = targets_[i].x_merge_(1);

    double tv = targets_[i].x_merge_(2);
    double tyaw = targets_[i].x_merge_(3);
    double tyaw_rate = targets_[i].x_merge_(4);

    // std::cout << "makeOutput: tx: " << tx << "; ty: " << ty << std::endl; // @DEBUG

    while (tyaw > M_PI)
      tyaw -= 2. * M_PI;
    while (tyaw < -M_PI)
      tyaw += 2. * M_PI;

    tf2::Quaternion q_tmp;
    q_tmp.setRPY(0, 0, tyaw);
    tf2::Quaternion q = q_tmp;

    nif_msgs::msg::DetectedObject dd;
    dd = targets_[i].object_;
    dd.id = targets_[i].ukf_id_;

    // @DEBUG velocity in global constrained to >= 0.0
    dd.velocity.linear.x = std::max(0.0, std::min(tv, 100.0)); 
    dd.acceleration.linear.y = tyaw_rate;
    dd.velocity_reliable = targets_[i].is_stable_;
    dd.pose_reliable = targets_[i].is_stable_;

    if (!targets_[i].is_static_ && targets_[i].is_stable_) {
      // Aligh the longest side of dimentions with the estimated orientation
      if (targets_[i].object_.dimensions.x < targets_[i].object_.dimensions.y) {
        dd.dimensions.x = targets_[i].object_.dimensions.y;
        dd.dimensions.y = targets_[i].object_.dimensions.x;
      }

      dd.pose.position.x = tx;
      dd.pose.position.y = ty;

      if (!std::isnan(q[0]))
        dd.pose.orientation.x = q[0];
      if (!std::isnan(q[1]))
        dd.pose.orientation.y = q[1];
      if (!std::isnan(q[2]))
        dd.pose.orientation.z = q[2];
      if (!std::isnan(q[3]))
        dd.pose.orientation.w = q[3];
    }
    updateBehaviorState(targets_[i], dd);

  // @DEBUG revertme
  // STABLE || OCCLUSION
    if (targets_[i].is_stable_ ||
        (targets_[i].tracking_num_ > TrackingState::Init &&
         targets_[i].tracking_num_ < TrackingState::Stable)) {
      tmp_objects.objects.push_back(dd);
      used_targets_indices.push_back(i);
    }
  }
  detected_objects_output =
      removeRedundantObjects(tmp_objects, used_targets_indices);
}

void ImmUkfPda::removeUnnecessaryTarget() {
  std::vector<UKF> temp_targets;
  for (size_t i = 0; i < targets_.size(); i++) {
    if (targets_[i].tracking_num_ != TrackingState::Die) {
      temp_targets.push_back(targets_[i]);
    }
    // @DEBUG Single tracker needs re-init
    else {
      init_ = false;
    }
  }
  std::vector<UKF>().swap(targets_);
  targets_ = temp_targets;
}

void ImmUkfPda::dumpResultText(
    nif_msgs::msg::DetectedObjectArray &detected_objects) {
  std::ofstream outputfile(result_file_path_,
                           std::ofstream::out | std::ofstream::app);
  for (size_t i = 0; i < detected_objects.objects.size(); i++) {
    double yaw = tf2::getYaw(detected_objects.objects[i].pose.orientation);

    // KITTI tracking benchmark data format:
    // (frame_number,tracked_id, object type, truncation, occlusion,
    // observation angle, x1,y1,x2,y2, h, w, l, cx, cy, cz, yaw) x1, y1, x2,
    // y2 are for 2D bounding box. h, w, l, are for height, width, length
    // respectively cx, cy, cz are for object centroid

    // Tracking benchmark is based on frame_number, tracked_id,
    // bounding box dimentions and object pose(centroid and orientation) from
    // bird-eye view
    outputfile << std::to_string(frame_count_) << " "
               << std::to_string(detected_objects.objects[i].id) << " "
               << "Unknown"
               << " "
               << "-1"
               << " "
               << "-1"
               << " "
               << "-1"
               << " "
               << "-1 -1 -1 -1"
               << " "
               << std::to_string(detected_objects.objects[i].dimensions.x)
               << " "
               << std::to_string(detected_objects.objects[i].dimensions.y)
               << " "
               << "-1"
               << " "
               << std::to_string(detected_objects.objects[i].pose.position.x)
               << " "
               << std::to_string(detected_objects.objects[i].pose.position.y)
               << " "
               << "-1"
               << " " << std::to_string(yaw) << "\n";
  }
  frame_count_++;
}

void ImmUkfPda::tracker(
    const nif_msgs::msg::DetectedObjectArray &input,
    nif_msgs::msg::DetectedObjectArray &detected_objects_output) {

  rclcpp::Time timestamp = input.header.stamp;
  std::vector<bool> matching_vec(input.objects.size(), false);

  if (!init_) {
    initTracker(input, timestamp);
    makeOutput(input, matching_vec, detected_objects_output);
    return;
  }

  rclcpp::Duration dt = timestamp - timestamp_;
  timestamp_ = timestamp;

  // start UKF process
  for (size_t i = 0; i < targets_.size(); i++) {
    targets_[i].is_stable_ = false;
    targets_[i].is_static_ = false;

    if (targets_[i].tracking_num_ == TrackingState::Die) {
      continue;
    }
    // prevent ukf not to explode
    if (targets_[i].p_merge_.determinant() > prevent_explosion_thres_ ||
        targets_[i].p_merge_(4, 4) > prevent_explosion_thres_) {
      targets_[i].tracking_num_ = TrackingState::Die;
      continue;
    }

    targets_[i].prediction(use_sukf_, has_subscribed_vectormap_, dt);

    std::vector<nif_msgs::msg::DetectedObject> object_vec;
    bool success = probabilisticDataAssociation(input, dt, matching_vec,
                                                object_vec, targets_[i]);

    if (!success) {
      continue;
    }

    targets_[i].update(use_sukf_, detection_probability_, gate_probability_,
                       gating_thres_, object_vec);
  }
  // end UKF process

  // making new ukf target for no data association objects
  // @DEBUG comment for single target
  // makeNewTargets(timestamp, input, matching_vec);

  // static dynamic classification
  // staticClassification();

  // making output for visualization
  makeOutput(input, matching_vec, detected_objects_output);

  // remove unnecessary ukf object
  removeUnnecessaryTarget();
}
