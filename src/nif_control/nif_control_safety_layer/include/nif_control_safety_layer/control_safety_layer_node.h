//
// Created by usrg on 6/23/21.
//

#ifndef ROS2MASTER_CONTROL_SAFETY_LAYER_NODE_H
#define ROS2MASTER_CONTROL_SAFETY_LAYER_NODE_H

#include "rclcpp/rclcpp.hpp"
#include "nif_common_nodes/base_synchronized_node.h"

namespace nif {
namespace control {

class ControlSafetyLayerNode : public nif::common::IBaseSynchronizedNode {
public:
    /**
     * Initialize ControlSafetyNodeLayer with default period (defined by IBaseSynchronizedNode)
     * @param node_name
     * @param options
     */
    ControlSafetyLayerNode(const std::string &node_name, const rclcpp::NodeOptions &options);

    /**
     * Initialize ControlSafetyNodeLayer with custom period.
     * @param node_name
     * @param options
     * @param period Custom synchronization period. It's passed to IBaseSynchronizedNode and determines the frequency run() is called at.
     */
    ControlSafetyLayerNode(const std::string &node_name, const rclcpp::NodeOptions &options, const std::chrono::duration<DurationRepT, DurationT> period);

protected:

private:
    /**
     * Stores control commands coming from the controllers' stack. It's flushed at every iteration by run(), that is it must store only the controls relative to a time quantum.
     */
    OrderedBuffer<ControlCmd> control_buffer;

    /**
     * Subscriber to the topic of control commands. Each incoming command is then saved in the buffer (should check the age).
     */
    rclcpp::Subscription<ControlCmd>::SharedPtr control_sub;

    /**
     * Control publisher. Publishes the effective command to the vehicle interface topic.
     */
    rclcpp::Publisher<ControlCmd>::SharedPtr control_pub;

    void controlCallback(const ControlCmd::SharedPtr & msg);
};

}
}

#endif // ROS2MASTER_CONTROL_SAFETY_LAYER_NODE_H
