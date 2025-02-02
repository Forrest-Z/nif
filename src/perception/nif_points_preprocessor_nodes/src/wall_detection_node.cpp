
/*
 * wall_detection_node.cpp
 *
 *  Created on: Oct 6, 2021
 *      Author: Daegyu Lee
 */
#include <nif_points_preprocessor_nodes/wall_detection_node.h>
#include "nif_frame_id/frame_id.h"
#include <numeric>
#ifdef GPU_CLUSTERING

#include "nif_points_preprocessor_nodes/gpu_euclidean_clustering.h"

#endif

using namespace nif::perception;
using namespace nif::common::frame_id::localization;

WallDetectionNode::WallDetectionNode(const std::string &node_name_)
    : Node(node_name_) {
  this->declare_parameter<double>("rear_upper_distance", double(50.0));
  this->declare_parameter<double>("front_upper_distance", double(50.0));

  this->declare_parameter<double>("normal_angle_thres", double(50.));
  this->declare_parameter<int>("ransac_pts_thresh", int(200));
  this->declare_parameter<double>("ransac_distance_thres", double(0.2));

  this->declare_parameter<double>("x_roi", double(0.));
  this->declare_parameter<double>("distance_extract_thres", double(0.5));
  this->declare_parameter<double>("distance_low_fass_filter", double(0.5));
  this->declare_parameter<double>("target_space_to_wall", double(7.));

  //cluster params
  this->declare_parameter<int>("cluster_size_min", 10);
  this->declare_parameter<int>("cluster_size_max", 2000);
  this->declare_parameter<double>("max_cluster_distance", 0.5);

  // Controller Parameters
  this->declare_parameter<int>("average_filter_size", int(10));
  this->declare_parameter<double>("min_lookahead", double(4.0));
  this->declare_parameter<double>("max_lookahead", double(30.0));
  this->declare_parameter<double>("lookahead_speed_ratio", double(0.75));
  this->declare_parameter<double>("proportional_gain", double(0.2));
  this->declare_parameter<double>("vehicle.wheelbase", double(2.97));
  this->declare_parameter<double>("max_steer_angle", double(30.0)); // 15 deg * 2 because ratio is wrong

  respond();


  RCLCPP_INFO(this->get_logger(), "normal_angle_thres: %f", normal_angle_thres_);
  RCLCPP_INFO(this->get_logger(), "ransac_pts_thresh: %f", ransac_pts_thresh_);
  RCLCPP_INFO(this->get_logger(), "x_roi : %f", extract_distance_x_roi);
  RCLCPP_INFO(this->get_logger(), "distance_extract_thres : %f", extract_distance_thres );
  RCLCPP_INFO(this->get_logger(), "distance_low_fass_filter : %f", distance_low_fass_filter );
  RCLCPP_INFO(this->get_logger(), "target_space_to_wall : %f", m_target_space_to_wall );

  sub_inverse_left_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
      "/inverse_left_mapped_points",
      nif::common::constants::QOS_SENSOR_DATA,
      std::bind(&WallDetectionNode::InverseLeftCallback, this,
                std::placeholders::_1));

  sub_inverse_right_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
      "/inverse_right_mapped_points", nif::common::constants::QOS_SENSOR_DATA,
      std::bind(&WallDetectionNode::InverseRightCallback, this,
                std::placeholders::_1));

  sub_wheel_speed_ = this->create_subscription<raptor_dbw_msgs::msg::WheelSpeedReport>(
      "raptor_dbw_interface/wheel_speed_report",
      nif::common::constants::QOS_SENSOR_DATA,
      std::bind(&WallDetectionNode::WheelSpeedCallback, this,
                std::placeholders::_1));
  sub_radar_marker_ =
      this->create_subscription<visualization_msgs::msg::Marker>(
          "/radar_front/radar_visz_moving",
          nif::common::constants::QOS_SENSOR_DATA,
          std::bind(&WallDetectionNode::RadarMarkerCallback, this,
                    std::placeholders::_1));

  using namespace std::chrono_literals; // NOLINT
  // TODO convert period to paramter
  timer_ = this->create_wall_timer(
      25ms, std::bind(&WallDetectionNode::timer_callback, this));

  pub_left_ransac_filtered_points =
      this->create_publisher<sensor_msgs::msg::PointCloud2>(
          "/ransac_filtered_points/left", nif::common::constants::QOS_SENSOR_DATA);
  pub_right_ransac_filtered_points =
      this->create_publisher<sensor_msgs::msg::PointCloud2>(
          "/ransac_filtered_points/right", nif::common::constants::QOS_SENSOR_DATA);
  pub_both_ransac_filtered_points =
      this->create_publisher<sensor_msgs::msg::PointCloud2>(
          "/ransac_filtered_points/both", nif::common::constants::QOS_SENSOR_DATA);

  pub_left_wall_line =
      this->create_publisher<nav_msgs::msg::Path>("/wall_left", nif::common::constants::QOS_SENSOR_DATA);
  pub_right_wall_line =
      this->create_publisher<nav_msgs::msg::Path>("/wall_right", nif::common::constants::QOS_SENSOR_DATA);
  pub_wall_following_path = this->create_publisher<nav_msgs::msg::Path>(
      "/wall_based_predictive_path", nif::common::constants::QOS_SENSOR_DATA);

  pub_inner_wall_distance = this->create_publisher<std_msgs::msg::Float32>(
      "/detected_inner_distance", nif::common::constants::QOS_SENSOR_DATA);
  pub_outer_wall_distance = this->create_publisher<std_msgs::msg::Float32>(
      "/detected_outer_distance", nif::common::constants::QOS_SENSOR_DATA);

  pub_wall_following_steer_cmd = this->create_publisher<std_msgs::msg::Float32>(
      "/wall_following_steering_cmd",
      nif::common::constants::QOS_CONTROL_CMD); // TODO


  // kin controller
  SetControllerParams();
  m_KinController.setCmdsToZeros();
}

WallDetectionNode::~WallDetectionNode() {}

void WallDetectionNode::respond() {
  this->get_parameter("rear_upper_distance", rear_upper_distance_);
  this->get_parameter("front_upper_distance", front_upper_distance_);
  
  this->get_parameter("normal_angle_thres", normal_angle_thres_);
  this->get_parameter("ransac_pts_thresh", ransac_pts_thresh_);
  this->get_parameter("ransac_distance_thres",m_ransacDistanceThres);

  this->get_parameter("x_roi", extract_distance_x_roi);
  this->get_parameter("distance_extract_thres", extract_distance_thres);
  this->get_parameter("distance_low_fass_filter", distance_low_fass_filter);
  this->get_parameter("target_space_to_wall", m_target_space_to_wall);

  m_average_filter_size = this->get_parameter("average_filter_size").as_int();

  this->m_cluster_size_min = this->get_parameter("cluster_size_min").as_int();
  this->m_cluster_size_max = this->get_parameter("cluster_size_max").as_int();
  this->m_max_cluster_distance =
      this->get_parameter("max_cluster_distance").as_double();
}

void WallDetectionNode::SetControllerParams() {
  m_KinController.min_la = this->get_parameter("min_lookahead").as_double();
  m_KinController.max_la = this->get_parameter("max_lookahead").as_double();
  m_KinController.la_ratio = this->get_parameter("lookahead_speed_ratio").as_double();
  m_KinController.Kp = this->get_parameter("proportional_gain").as_double();
  m_KinController.L = this->get_parameter("vehicle.wheelbase").as_double();
  m_KinController.ms = this->get_parameter("max_steer_angle").as_double();
}


void WallDetectionNode::timer_callback() {
  pcl::PointCloud<pcl::PointXYZI>::Ptr CloudLeftWallPoints(
      new pcl::PointCloud<pcl::PointXYZI>);
  pcl::PointCloud<pcl::PointXYZI>::Ptr CloudRightWallPoints(
      new pcl::PointCloud<pcl::PointXYZI>);

  bool left_wall_detected = false;
  bool right_wall_detected = false;

  RCLCPP_DEBUG(this->get_logger(), "1");

  if (bInverseLeftPoints) {
    /* Wall-clustering FILTER
    1. Implement left/right points separately.
      - input : left ground filtered points
      - output : distance to the wall, filtered left wall points
    */
    // left wall detection
    RCLCPP_DEBUG(this->get_logger(), "a");

    wall_detect_with_clustering(m_InverseLeftPoints, CloudLeftWallPoints,
                       m_max_cluster_distance);
    RCLCPP_DEBUG(this->get_logger(), "b");
    inner_bound_distance = 0;
    outer_bound_distance = 0;
    // detected left_wall_plane_coeff coefficients
    if (!CloudLeftWallPoints->points.empty()) {
      RCLCPP_DEBUG(this->get_logger(), "Left margin : %f", inner_bound_distance);

      ExtractDistanceInCloud(CloudLeftWallPoints, extract_distance_x_roi,
                            extract_distance_thres, inner_bound_distance);
      left_wall_detected = true;
    }
    sensor_msgs::msg::PointCloud2 cloud_left_ransac_filtered_msg;
    pcl::toROSMsg(*CloudLeftWallPoints, cloud_left_ransac_filtered_msg);
    cloud_left_ransac_filtered_msg.header.frame_id = BASE_LINK;
    cloud_left_ransac_filtered_msg.header.stamp = this->now();
    pub_left_ransac_filtered_points->publish(cloud_left_ransac_filtered_msg);
    std_msgs::msg::Float32 inner_bound_distance_msg;
    inner_bound_distance_msg.data =
        -1 * ((distance_low_fass_filter)*inner_bound_distance +
              (1 - distance_low_fass_filter) * prev_inner_bound_distance);
    pub_inner_wall_distance->publish(inner_bound_distance_msg);
    prev_inner_bound_distance = inner_bound_distance;
  }
  RCLCPP_DEBUG(this->get_logger(), "2");

  if (bInverseRightPoints)
  {
    /* RIGHT WALL BASED
    1. Remove outlier points
    2. Implement left/right points separately.
      - input : right ground filtered points
      - output : distance to the wall, ransac filtered right wall points
    */
    // right wall detection
    RCLCPP_DEBUG(this->get_logger(), "a");
    wall_detect_with_clustering(m_InverseRightPoints, CloudRightWallPoints,
                                m_max_cluster_distance);
    RCLCPP_DEBUG(this->get_logger(), "b");

    // detected right_wall_plane_coeff coefficients
    if (!CloudRightWallPoints->points.empty()) {
      RCLCPP_DEBUG(this->get_logger(), "Right margin : %f",
                  -1 * outer_bound_distance);
      ExtractDistanceInCloud(CloudRightWallPoints, extract_distance_x_roi,
                            extract_distance_thres, outer_bound_distance);
      right_wall_detected = true;
    }
    sensor_msgs::msg::PointCloud2 cloud_right_ransac_filtered_msg;
    pcl::toROSMsg(*CloudRightWallPoints, cloud_right_ransac_filtered_msg);
    cloud_right_ransac_filtered_msg.header.frame_id = BASE_LINK;
    cloud_right_ransac_filtered_msg.header.stamp = this->now();
    pub_right_ransac_filtered_points->publish(cloud_right_ransac_filtered_msg);

    std_msgs::msg::Float32 outer_bound_distance_msg;
    // outer_bound_distance_msg.data =
    //     -1 * ((distance_low_fass_filter)*outer_bound_distance +
    //           (1 - distance_low_fass_filter) * prev_outer_bound_distance);
    outer_bound_distance_msg.data = -1 * outer_bound_distance;

    pub_outer_wall_distance->publish(outer_bound_distance_msg);

    prev_outer_bound_distance = outer_bound_distance;
  }
  RCLCPP_DEBUG(this->get_logger(), "3");

  /* BOTH OF RANSAC-FILTERED POINTS PUBLISHER
  1. Publish ransac filtered both of wall points
    - input : left/right ransac filtered points
    - output : both of ransac filtered points
  */
  pcl::PointCloud<pcl::PointXYZI>::Ptr CloudWallBoth(
      new pcl::PointCloud<pcl::PointXYZI>);
  *CloudWallBoth += *CloudRightWallPoints;
  *CloudWallBoth += *CloudLeftWallPoints;

  sensor_msgs::msg::PointCloud2 cloud_both_ransac_filtered_msg;
  pcl::toROSMsg(*CloudWallBoth, cloud_both_ransac_filtered_msg);
  cloud_both_ransac_filtered_msg.header.frame_id = BASE_LINK;
  cloud_both_ransac_filtered_msg.header.stamp = this->now();
  pub_both_ransac_filtered_points->publish(cloud_both_ransac_filtered_msg);

  RCLCPP_DEBUG(this->get_logger(), "4");
  /* CUBIC SPLINER & WALL PATH PUBLISHER
  1. Publish left/right wall detected path on the BASE_LINK frame
    - input : left/right ransac-filtered points
    - output : ransac-filtered wall path
  */
  nav_msgs::msg::Path left_path_msg;
  left_path_msg.header.frame_id = BASE_LINK;
  left_path_msg.header.stamp = this->now();
  cv::Mat LeftPolyCoefficient;

  CubicSpliner(CloudLeftWallPoints, left_wall_detected, front_upper_distance_,
               rear_upper_distance_, left_path_msg, LeftPolyCoefficient);

  pub_left_wall_line->publish(left_path_msg);

  nav_msgs::msg::Path right_path_msg;
  right_path_msg.header.frame_id = BASE_LINK;
  right_path_msg.header.stamp = this->now();
  cv::Mat RightPolyCoefficient;

  CubicSpliner(CloudRightWallPoints, right_wall_detected, front_upper_distance_,
               rear_upper_distance_, right_path_msg, RightPolyCoefficient);
  pub_right_wall_line->publish(right_path_msg);

  RCLCPP_DEBUG(this->get_logger(), "5");
  /* WALL CURVATURE BASED CONTROLLER
  1. Left/Right wall based lateral control output
  2. Estimated curvature / predictive path based on the kinematic model
    - input : coefficient of cubic splined wall
    - input : feasibility of wall detection result(named as right_wall_detected/left_wall_detected)
    - output : wall following reference path on the base_link frame
  */
  nav_msgs::msg::Path wall_folllowing_path_msg;
  wall_folllowing_path_msg.header.frame_id = BASE_LINK;
  wall_folllowing_path_msg.header.stamp = this->now();
  bool close_to_left = false;
  if (right_wall_detected && outer_bound_distance != 0.0) {
    if (fabs(outer_bound_distance) < m_target_space_to_wall)
    {
      m_margin_to_wall = outer_bound_distance + m_target_space_to_wall;
    }
    else {
      m_margin_to_wall = 0.0;
    }
  }

  if (left_wall_detected && inner_bound_distance != 0.0) {
    if (fabs(inner_bound_distance) < m_target_space_to_wall) {
      m_margin_to_wall = inner_bound_distance - m_target_space_to_wall;
      close_to_left = true;
    } else {
      m_margin_to_wall = 0.0;
    }
  }

  // if vehicle goes close to the left wall, calculate predictive path from left wall, not from the right wall. 
  if(close_to_left)
  {
    EstimatePredictivePath(left_wall_detected, LeftPolyCoefficient,
                           wall_folllowing_path_msg, m_margin_to_wall);
  }
  else
  {
    EstimatePredictivePath(right_wall_detected, RightPolyCoefficient,
                           wall_folllowing_path_msg, m_margin_to_wall);
  }

  final_wall_following_path_msg.header.frame_id = BASE_LINK;
  final_wall_following_path_msg.header.stamp = this->now();
  if (right_wall_detected && !wall_folllowing_path_msg.poses.empty()) {
    final_wall_following_path_msg = wall_folllowing_path_msg;
  }
  if (left_wall_detected && !wall_folllowing_path_msg.poses.empty() &&
      close_to_left) {
    final_wall_following_path_msg = wall_folllowing_path_msg;
  }
  pub_wall_following_path->publish(final_wall_following_path_msg);

  RCLCPP_DEBUG(this->get_logger(), "6");
  /* KIN-CONTROLLER 
  1. Calculate Control output using KinController
  2. Calculate predictive path from the vehicle
    - params : getparam using SetControllerParams()
    - input : wall_folllowing_path_msg
    - output : control output
    - output : predictive path on the base_link frame
  */

  m_KinController.setPath(final_wall_following_path_msg);
  m_KinController.setVelocity(m_vel_speed_x);
  m_KinController.run();


  double wall_following_steering_cmd = 0.0;
  m_KinController.getSteering(wall_following_steering_cmd);
  
  control_out_que.push_back(wall_following_steering_cmd);
  if(control_out_que.size() > m_average_filter_size)
  {
    control_out_que.pop_front();
  }
  size_t filter_que_size = control_out_que.size();

  double filter_que_sum = 0.0;
  for(auto cmd : control_out_que)
  {
    filter_que_sum += cmd;
  }
  double filtered_steering_output = filter_que_sum / filter_que_size;
  // if vehicle is not close to the outer wall, but steering output is negative, cut steering cmd as zero.
  // normally false negative output was calculated when vehicle proceeds from straight to curvy road.
  // However, if vehicle is close to the left wall, this saturator does not work.    
  if (!close_to_left && filtered_steering_output < 0.)
  {
    filtered_steering_output = 0.0 ;
  }

  std_msgs::msg::Float32 wall_following_steering_cmd_msg;
  wall_following_steering_cmd_msg.data = filtered_steering_output;
  pub_wall_following_steer_cmd->publish(wall_following_steering_cmd_msg);

}

void WallDetectionNode::InverseLeftCallback(
    const sensor_msgs::msg::PointCloud2::SharedPtr msg) {
  std::lock_guard<std::mutex> sensor_lock(mtx_left);
  m_InverseLeftPoints.reset(new pcl::PointCloud<pcl::PointXYZI>());
  pcl::fromROSMsg(*msg, *m_InverseLeftPoints);
  bInverseLeftPoints = true;
}
void WallDetectionNode::InverseRightCallback(
    const sensor_msgs::msg::PointCloud2::SharedPtr msg) {
  std::lock_guard<std::mutex> sensor_lock(mtx_right);
  m_InverseRightPoints.reset(new pcl::PointCloud<pcl::PointXYZI>());
  pcl::fromROSMsg(*msg, *m_InverseRightPoints);
  bInverseRightPoints = true;
}

void WallDetectionNode::WheelSpeedCallback(const raptor_dbw_msgs::msg::WheelSpeedReport::SharedPtr msg)
{
  m_vel_speed_x = (msg->front_right + msg->front_left) / 2 * nif::common::constants::KPH2MS;
}

void WallDetectionNode::RadarMarkerCallback(
    const visualization_msgs::msg::Marker::SharedPtr msg) {

}

pcl::PointCloud<pcl::PointXYZI>::Ptr
WallDetectionNode::downsample(pcl::PointCloud<pcl::PointXYZI>::Ptr cloud,
                               double resolution) {
  pcl::PointCloud<pcl::PointXYZI>::Ptr filtered(
      new pcl::PointCloud<pcl::PointXYZI>);
  pcl::VoxelGrid<pcl::PointXYZI> voxelgrid;
  voxelgrid.setLeafSize(resolution, resolution, 0.05);
  voxelgrid.setInputCloud(cloud);
  voxelgrid.filter(*filtered);
  return filtered;
}

void WallDetectionNode::ExtractDistanceInCloud(
    const pcl::PointCloud<pcl::PointXYZI>::Ptr &cloudIn, const double& x_roi_, const double& dist_thres_, double& distance_out)
    {
      double sum_y = 0;
      double count = 0;
      for(auto point : cloudIn->points)
      {
        if (fabs(point.x - x_roi_) < dist_thres_) {
          sum_y += point.y;
          count ++;
        }
      }
      if(count != 0)
      {
        distance_out = sum_y / count;
      }
      else
      {
        distance_out = 0;
      }

    }

void WallDetectionNode::CubicSpliner(
        pcl::PointCloud<pcl::PointXYZI>::Ptr cloudIn,
        const bool wall_detected,
        double in_front_upper_threshold, double in_rear_upper_threshold,
        nav_msgs::msg::Path& path_msg_out, cv::Mat& PolyCoefficientOut) {
  if (cloudIn->points.empty())
    return;

  pcl::PointCloud<pcl::PointXYZI>::Ptr cloudDownSampled(
      new pcl::PointCloud<pcl::PointXYZI>);

  for (auto point_buf : cloudIn->points) {
    pcl::PointXYZI ground_points;
    ground_points.x = point_buf.x;
    ground_points.y = point_buf.y;
    cloudDownSampled->points.push_back(ground_points);
  }
  cloudDownSampled = downsample(cloudDownSampled, 0.25);

  std::vector<cv::Point2f> ToBeFit;
  ToBeFit.clear();

  for (auto point_buf : cloudDownSampled->points) {
    cv::Point2f pointTmp;
    pointTmp.x = point_buf.x;
    pointTmp.y = point_buf.y;
    ToBeFit.push_back(pointTmp);
  }

  // For polynomial fitting
  int poly_order = 2;
  PolyCoefficientOut = polyfit(ToBeFit, poly_order);

  if (!wall_detected) {
    return;
  }

  for (double x = -30; x < 50; x = x + 0.5) {
    geometry_msgs::msg::PoseStamped pose_buf;
    pose_buf.pose.position.x = x;
    pose_buf.pose.position.y =
        PolyCoefficientOut.at<double>(poly_order, 0) * pow(x, poly_order) +
        PolyCoefficientOut.at<double>(poly_order - 1, 0) * pow(x, poly_order - 1) +
        // PolyCoefficientOut.at<double>(poly_order - 2, 0) * pow(x, poly_order - 2) +
        PolyCoefficientOut.at<double>(poly_order - 2, 0); // Cubic
    pose_buf.pose.orientation.w = 1;
    pose_buf.pose.orientation.x = 0;
    pose_buf.pose.orientation.y = 0;
    pose_buf.pose.orientation.z = 0;
    path_msg_out.poses.push_back(pose_buf);
  }
}


cv::Mat WallDetectionNode::polyfit(std::vector<cv::Point2f> &in_point, int n) {
  int size = in_point.size();

  int x_num = n + 1;

  cv::Mat mat_u(size, x_num, CV_64F);
  cv::Mat mat_y(size, 1, CV_64F);
  cv::Mat mat_k(x_num, 1, CV_64F);

  if (size == 0) {
    for (int i = 0; i < mat_k.rows; ++i) {
      mat_k.at<double>(i, 0) = 0;
    }
    return mat_k;
  } else {
    for (int i = 0; i < mat_u.rows; ++i)
      for (int j = 0; j < mat_u.cols; ++j) {
        mat_u.at<double>(i, j) = pow(in_point[i].x, j);
      }

    for (int i = 0; i < mat_y.rows; ++i) {
      mat_y.at<double>(i, 0) = in_point[i].y;
    }

    mat_k = (mat_u.t() * mat_u).inv() * mat_u.t() * mat_y;

    return mat_k;
  }
}

void WallDetectionNode::EstimatePredictivePath(
    const bool wall_detected, const cv::Mat &PolyCoefficientIn,
    nav_msgs::msg::Path &path_msg_out, const double &target_space_to_wall) {
  if (!wall_detected) {
    return;
  }
  
  int poly_order = 2;
  for (double x = -10; x < 50; x = x + 0.5) {
    geometry_msgs::msg::PoseStamped pose_buf;
    pose_buf.pose.position.x = x;
    pose_buf.pose.position.y =
        PolyCoefficientIn.at<double>(poly_order, 0) * pow(x, poly_order) +
        PolyCoefficientIn.at<double>(poly_order - 1, 0) * pow(x, poly_order - 1) +
        // PolyCoefficientIn.at<double>(poly_order - 2, 0) * pow(x, poly_order - 2) +
        target_space_to_wall; // Cubic

    pose_buf.pose.orientation.w = 1;
    pose_buf.pose.orientation.x = 0;
    pose_buf.pose.orientation.y = 0;
    pose_buf.pose.orientation.z = 0;
    path_msg_out.poses.push_back(pose_buf);
  }
}

void WallDetectionNode::wall_detect_with_clustering(
    const pcl::PointCloud<pcl::PointXYZI>::Ptr in_cloud_ptr,
    pcl::PointCloud<pcl::PointXYZI>::Ptr out_cloud_ptr,
    double in_max_cluster_distance) {

  RCLCPP_DEBUG(this->get_logger(), "x");

  std::lock_guard<std::mutex> right_lock(mtx_right);
  std::lock_guard<std::mutex> left_lock(mtx_left);

#ifdef GPU_CLUSTERING

  int size = in_cloud_ptr->points.size();

  // if (size == 0)
  //   return clusters;

  float *tmp_x, *tmp_y, *tmp_z;

  tmp_x = (float *)malloc(sizeof(float) * size);
  tmp_y = (float *)malloc(sizeof(float) * size);
  tmp_z = (float *)malloc(sizeof(float) * size);

  for (int i = 0; i < size; i++) {
    pcl::PointXYZI tmp_point = in_cloud_ptr->at(i);

    tmp_x[i] = tmp_point.x;
    tmp_y[i] = tmp_point.y;
    tmp_z[i] = tmp_point.z;
  }
  RCLCPP_DEBUG(this->get_logger(), "y");

  GpuEuclideanCluster gecl_cluster;
  RCLCPP_DEBUG(this->get_logger(), "y1");
  gecl_cluster.setInputPoints(tmp_x, tmp_y, tmp_z, size);
  RCLCPP_DEBUG(this->get_logger(), "y2");
  gecl_cluster.setThreshold(in_max_cluster_distance);
  RCLCPP_DEBUG(this->get_logger(), "y3");

  gecl_cluster.setMinClusterPts(m_cluster_size_min);
  RCLCPP_DEBUG(this->get_logger(), "y4");

  gecl_cluster.setMaxClusterPts(m_cluster_size_max);
  RCLCPP_DEBUG(this->get_logger(), "y5");

  gecl_cluster.extractClusters();
  RCLCPP_DEBUG(this->get_logger(), "y6");

  std::vector<GpuEuclideanCluster::GClusterIndex> cluster_indices =
      gecl_cluster.getOutput();
  RCLCPP_DEBUG(this->get_logger(), "y7");

  unsigned int k = 0;
  double max_length = 0;
  RCLCPP_DEBUG(this->get_logger(), "z");
  pcl::PointCloud<pcl::PointXYZI>::Ptr FinalPoints(new pcl::PointCloud<pcl::PointXYZI>);
  for (auto it = cluster_indices.begin(); it != cluster_indices.end(); it++) {
    pcl::PointCloud<pcl::PointXYZI>::Ptr bufPoints(
        new pcl::PointCloud<pcl::PointXYZI>);
    double cur_length;
    SetCloud(in_cloud_ptr, bufPoints, it->points_in_cluster, k, cur_length);
    //   clusters.push_back(cluster);
    k++;
    if (cur_length > max_length)
    {
      max_length = cur_length;
      FinalPoints.reset(new pcl::PointCloud<pcl::PointXYZI>());
      *FinalPoints = *bufPoints;
    }
  }
  *out_cloud_ptr  =  *FinalPoints;
  
  RCLCPP_DEBUG(this->get_logger(), "w");

  // std::cout << "working" << std::endl;

  free(tmp_x);
  free(tmp_y);
  free(tmp_z);

  RCLCPP_DEBUG(this->get_logger(), "o");

#endif
  // return clusters;
}

void WallDetectionNode::SetCloud(
    const pcl::PointCloud<pcl::PointXYZI>::Ptr in_origin_cloud_ptr,
    pcl::PointCloud<pcl::PointXYZI>::Ptr register_cloud_ptr,
    const std::vector<int> &in_cluster_indices, int ind,
    double &cluster_length) {

  pcl::PointCloud<pcl::PointXYZI>::Ptr bufPoints(
      new pcl::PointCloud<pcl::PointXYZI>);
  // extract pointcloud using the indices
  // calculate min and max points
  float min_x = std::numeric_limits<float>::max();
  float max_x = -std::numeric_limits<float>::max();
  float min_y = std::numeric_limits<float>::max();
  float max_y = -std::numeric_limits<float>::max();
  float min_z = std::numeric_limits<float>::max();
  float max_z = -std::numeric_limits<float>::max();
  float average_x = 0, average_y = 0, average_z = 0;

  for (auto pit = in_cluster_indices.begin(); pit != in_cluster_indices.end();
       ++pit) {
    // fill new colored cluster point by point
    pcl::PointXYZI p;
    p.x = in_origin_cloud_ptr->points[*pit].x;
    p.y = in_origin_cloud_ptr->points[*pit].y;
    p.z = in_origin_cloud_ptr->points[*pit].z;
    p.intensity = ind;

    average_x += p.x;
    average_y += p.y;
    average_z += p.z;
    bufPoints->points.push_back(p);

    if (p.x < min_x)
      min_x = p.x;
    if (p.y < min_y)
      min_y = p.y;
    if (p.z < min_z)
      min_z = p.z;
    if (p.x > max_x)
      max_x = p.x;
    if (p.y > max_y)
      max_y = p.y;
    if (p.z > max_z)
      max_z = p.z;
  }
  double length = max_x - min_x;
  double width = max_y - min_y;
  double height = max_z - min_z;

  // if (length < 7.0 && width < 7.0 && height > 0.5) {
  *register_cloud_ptr = *bufPoints;
  cluster_length = length;
  // }
}