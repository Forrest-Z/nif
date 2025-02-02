#ifndef LANE_TO_COSTMAP_H
#define LANE_TO_COSTMAP_H

// headers in ROS
#include <ros/ros.h>
#include <grid_map_ros/grid_map_ros.hpp>

// headers in PCL
#include <pcl_ros/point_cloud.h>

class LaneToCostmap
{
public:
  LaneToCostmap();
  ~LaneToCostmap();

  /// \brief calculate cost from sensor points
  /// \param[in] maximum_height_thres: Maximum height threshold for pointcloud data
  /// \param[in] minimum_height_thres: Minimum height threshold for pointcloud data
  /// \param[in] grid_min_value: Minimum cost for costmap
  /// \param[in] grid_max_value: Maximum cost fot costmap
  /// \param[in] gridmap: costmap based on gridmap
  /// \param[in] gridmap_layer_name: gridmap layer name for gridmap
  /// \param[in] in_sensor_points: subscribed pointcloud
  /// \param[out] calculated cost in grid_map::Matrix format
  grid_map::Matrix makeCostmapFromSensorPoints(const double maximum_height_thres, const double minimum_height_thres,
                                               const double grid_min_value, const double grid_max_value,
                                               const grid_map::GridMap& gridmap, const std::string& gridmap_layer_name,
                                               const pcl::PointCloud<pcl::PointXYZ>::Ptr& in_sensor_points);

private:

  double grid_length_x_;
  double grid_length_y_;
  double grid_resolution_;
  double grid_position_x_;
  double grid_position_y_;
  double y_cell_size_;
  double x_cell_size_;

  /// \brief initialize gridmap parameters
  /// \param[in] gridmap: gridmap object to be initialized
  void initGridmapParam(const grid_map::GridMap& gridmap);

  /// \brief check if index is valid in the gridmap
  /// \param[in] grid_ind: grid index corresponding with one of pointcloud
  /// \param[out] bool: true if index is valid
  bool isValidInd(const grid_map::Index& grid_ind);

  /// \brief Get index from one of pointcloud
  /// \param[in] point: one of subscribed pointcloud
  /// \param[out] index in gridmap
  grid_map::Index fetchGridIndexFromPoint(const pcl::PointXYZ& point);

  /// \brief Assign pointcloud to appropriate cell in gridmap
  /// \param[in] gridmap: costmap based on gridmap
  /// \param[in] in_sensor_points: subscribed pointcloud
  /// \param[out] grid-x-length x grid-y-length size grid stuffed with point's height in corresponding grid cell
  std::vector<std::vector<std::vector<double>>>
  assignPoints2GridCell(const grid_map::GridMap& gridmap, const pcl::PointCloud<pcl::PointXYZ>::Ptr& in_sensor_points);

  /// \brief calculate costmap from subscribed pointcloud
  /// \param[in] maximum_height_thres: Maximum height threshold for pointcloud data
  /// \param[in] minimum_height_thres: Minimum height threshold for pointcloud data
  /// \param[in] grid_min_value: Minimum cost for costmap
  /// \param[in] grid_max_value: Maximum cost fot costmap
  /// \param[in] gridmap: costmap based on gridmap
  /// \param[in] gridmap_layer_name: gridmap layer name for gridmap
  /// \param[in] grid_vec: grid-x-length x grid-y-length size grid stuffed with point's height in corresponding grid
  /// cell
  /// \param[out] caculated costmap in grid_map::Matrix format
  grid_map::Matrix calculateCostmap(const double maximum_height_thres, const double minimum_lidar_height_thres,
                                    const double grid_min_value, const double grid_max_value,
                                    const grid_map::GridMap& gridmap, const std::string& gridmap_layer_name,
                                    const std::vector<std::vector<std::vector<double>>> grid_vec);
};

#endif  // LANE_TO_COSTMAP_H
