//  Copyright (c) 2021 Unmanned System Research Group @ KAIST
//  Author:

//
// Created by usrg on 6/24/21.
//

#ifndef ROS2MASTER_PLANNER_NODE_H
#define ROS2MASTER_PLANNER_NODE_H

#include "nif_common/types.h"
#include "nif_common_nodes/i_base_node.h"
#include "nif_planning_common/i_planner_common.h"

namespace nif {
namespace planning {

class PlannerNode : public nif::common::IBaseNode {
public:
  PlannerNode(nif::planning::common::IPlannerAlgorithm& planner_algorithm_);

protected:
  /**
   * Subscribtion to opponents vehicles' states.
   */
  rclcpp::Subscription<nif::common::types::t_oppo_collection_states>
      opponents_state_sub;

  rclcpp::Subscription<nif::common::msgs::TerrainState> track_state_sub;

  rclcpp::Publisher<nif::common::msgs::Trajectory> trajectory_pub;

  nif::planning::common::IPlannerAlgorithm& planner_algorithm;

  void opponentsStateCallback(
      const nif::common::types::t_oppo_collection_states& msg);

  void
  terrainStateCallback(const nif::common::msgs::TerrainState::SharedPtr& msg);

private:
};

} // namespace planning
} // namespace nif

#endif // ROS2MASTER_PLANNER_NODE_H
