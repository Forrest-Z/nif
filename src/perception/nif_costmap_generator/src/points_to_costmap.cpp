#include "costmap_generator/points_to_costmap.h"

// Constructor
PointsToCostmap::PointsToCostmap() {}

PointsToCostmap::~PointsToCostmap() {}

void PointsToCostmap::initGridmapParam(const grid_map::GridMap &gridmap) {
  grid_length_x_ = gridmap.getLength().x();
  grid_length_y_ = gridmap.getLength().y();
  grid_resolution_ = gridmap.getResolution();
  grid_position_x_ = gridmap.getPosition().x();
  grid_position_y_ = gridmap.getPosition().y();
}

bool PointsToCostmap::isValidInd(const grid_map::Index &grid_ind) {
  bool is_valid = false;
  int x_grid_ind = grid_ind.x();
  int y_grid_ind = grid_ind.y();
  if (x_grid_ind >= 0 &&
      x_grid_ind < std::ceil(grid_length_x_ * (1 / grid_resolution_)) &&
      y_grid_ind >= 0 &&
      y_grid_ind < std::ceil(grid_length_y_ * (1 / grid_resolution_))) {
    is_valid = true;
  }
  return is_valid;
}

grid_map::Index
PointsToCostmap::fetchGridIndexFromPoint(const pcl::PointXYZI &point) {
  // calculate out_grid_map position
  const double origin_x_offset = grid_length_x_ / 2.0 - grid_position_x_;
  const double origin_y_offset = grid_length_y_ / 2.0 - grid_position_y_;
  // coordinate conversion for making index. Set bottom left to the origin of
  // coordinate (0, 0) in gridmap area
  double mapped_x =
      (grid_length_x_ - origin_x_offset - point.x) / grid_resolution_;
  double mapped_y =
      (grid_length_y_ - origin_y_offset - point.y) / grid_resolution_;

  int mapped_x_ind = std::floor(mapped_x);
  int mapped_y_ind = std::floor(mapped_y);
  grid_map::Index index(mapped_x_ind, mapped_y_ind);
  return index;
}

std::vector<std::vector<std::vector<double>>>
PointsToCostmap::assignPoints2GridCell(
    const grid_map::GridMap &gridmap,
    const pcl::PointCloud<pcl::PointXYZI>::Ptr &in_sensor_points) {
  double y_cell_size = std::ceil(grid_length_y_ * (1 / grid_resolution_));
  double x_cell_size = std::ceil(grid_length_x_ * (1 / grid_resolution_));
  std::vector<double> z_vec;
  std::vector<std::vector<double>> vec_y_z(y_cell_size, z_vec);
  std::vector<std::vector<std::vector<double>>> vec_x_y_z(x_cell_size, vec_y_z);

  for (const auto &point : *in_sensor_points) {
    grid_map::Index grid_ind = fetchGridIndexFromPoint(point);
    if (isValidInd(grid_ind)) {
      vec_x_y_z[grid_ind.x()][grid_ind.y()].push_back(point.z);
    }
  }
  return vec_x_y_z;
}

grid_map::Matrix PointsToCostmap::calculateCostmap(
    const double maximum_height_thres, const double minimum_lidar_height_thres,
    const double grid_min_value, const double grid_max_value,
    const grid_map::GridMap &gridmap, const std::string &gridmap_layer_name,
    const std::vector<std::vector<std::vector<double>>> grid_vec) {

  grid_map::Matrix gridmap_data = gridmap[gridmap_layer_name];
  gridmap_data.setConstant(0.0);
  for (size_t x_ind = 0; x_ind < grid_vec.size(); x_ind++) {
    for (size_t y_ind = 0; y_ind < grid_vec[0].size(); y_ind++) {
      if (grid_vec[x_ind][y_ind].size() == 0) {
        gridmap_data(x_ind, y_ind) = grid_min_value;
        continue;
      }
      for (const auto &z : grid_vec[x_ind][y_ind]) {
        if (z > maximum_height_thres || z < minimum_lidar_height_thres) {
          continue;
        }
        gridmap_data(x_ind, y_ind) = grid_max_value;
        break;
      }
    }
  }
  return gridmap_data;
}

grid_map::Matrix PointsToCostmap::makeCostmapFromSensorPoints(
    const double maximum_height_thres, const double minimum_lidar_height_thres,
    const double grid_min_value, const double grid_max_value,
    const grid_map::GridMap &gridmap, const std::string &gridmap_layer_name,
    const pcl::PointCloud<pcl::PointXYZI>::Ptr &in_sensor_points) {
  initGridmapParam(gridmap);
  std::vector<std::vector<std::vector<double>>> grid_vec =
      assignPoints2GridCell(gridmap, in_sensor_points);

  grid_map::Matrix costmap = calculateCostmap(
      maximum_height_thres, minimum_lidar_height_thres, grid_min_value,
      grid_max_value, gridmap, gridmap_layer_name, grid_vec);
  return costmap;
}


