import sys
from numpy.lib.polynomial import RankWarning
import pymap3d as pm
import numpy as np
import matplotlib.pyplot as plt
import sys, os
from ament_index_python.packages import get_package_share_directory

import csv
import pickle
import sys
import logging
# import graph_ltpl
import yaml
import matplotlib.patches as patches
import graphviz
import random

ax = plt.subplot(1,1,1)


def get_mission_label(mission_node):
    mission_label = r'' + \
        str(mission_code_block.get("mission_code")) + \
        ' |{ ' + str(mission_code_block.get("mission_code")) + \
            ' | | f}| g | h'
    return 
# fig2 = plt.figure(2)

class WPTFileVisualizer:
    def __init__(self, file_path, legend):

        self.wpt_x, self.wpt_y, self.wpt_yaw_rad = self.readTrackCenterFile(file_path)
        self.visualization(legend)


    def readTrackCenterFile(self, file_path_):
        path_x_list_ = []
        path_y_list_ = []
        path_yaw_list_ = []

        cnt = 0
        fileHandle = open(file_path_, "r")
        for line in fileHandle:
            cnt += 1
            fields = line.split(",")
            if fields[0] != " " and fields[1] != " ":
                x = float(fields[0])
                y = float(fields[1])
                yaw = float(fields[1])
                path_x_list_.append(x)
                path_y_list_.append(y)
                path_yaw_list_.append(yaw)
        fileHandle.close()

        # Check loaded data
        assert len(path_x_list_) == len(
            path_y_list_
        ), "path x and y length are different"
        assert len(path_x_list_) != 0, "path file empty"
        assert len(path_yaw_list_) != 0, "path file empty"

        return path_x_list_, path_y_list_, path_yaw_list_

    def visualization(self, legend):
        plt.plot(self.wpt_x, self.wpt_y, label = legend)
        plt.legend(loc="upper left")
        plt.axis('equal')
        plt.grid()

class RaceLineFileVisualizer:
    def __init__(self, file_path, legend):

        self.wpt_x, self.wpt_y, self.wpt_yaw_rad = self.readTrackCenterFile(file_path)
        self.visualization(legend)


    def readTrackCenterFile(self, file_path_):
        path_x_list_ = []
        path_y_list_ = []
        path_yaw_list_ = []

        cnt = 0
        fileHandle = open(file_path_, "r")
        for line in fileHandle:
            cnt += 1
            if(cnt > 3):
                fields = line.split(";")
                if fields[0] != " " and fields[1] != " ":
                    x = float(fields[1])
                    y = float(fields[2])
                    yaw = float(fields[3])
                    path_x_list_.append(x)
                    path_y_list_.append(y)
                    path_yaw_list_.append(yaw)
        fileHandle.close()

        # Check loaded data
        assert len(path_x_list_) == len(
            path_y_list_
        ), "path x and y length are different"
        assert len(path_x_list_) != 0, "path file empty"
        assert len(path_yaw_list_) != 0, "path file empty"

        return path_x_list_, path_y_list_, path_yaw_list_

    def visualization(self, legend):
        plt.plot(self.wpt_x, self.wpt_y, label = legend)
        plt.legend(loc="upper left")
        plt.axis('equal')
        plt.grid()

class GraphVisualizer:
    def __init__(self, file_path, legend):

        self.graph_path = file_path
        self.nodes = []
        self.nodes_pos = []
        self.loadGraph()
        self.visualization(legend)


    def loadGraph(self):
        f = open(self.graph_path, 'rb')
        graph_base = pickle.load(f)
        f.close()

        nodes= graph_base.get_nodes()
        self.nodes = nodes
        for i, node in enumerate(nodes):
            # get children and parents of node
            pos, _, raceline, children, parents = graph_base.get_node_info(layer=node[0],
                                                                    node_number=node[1],
                                                                    return_child=True,
                                                                    return_parent=True)
            self.nodes_pos.append(pos)

    def visualization(self, legend):
        node_x = []
        node_y = []
        for i in range(len(self.nodes_pos)):
            node_x.append(self.nodes_pos[i][0])
            node_y.append(self.nodes_pos[i][1])

        plt.scatter(node_x, node_y, s = 2.5 )

if __name__ == "__main__":

    TRACK_NAME = 'IMS'
    TRANSITION_FILE = 'transitions.ims.yaml'

    mission_manager_path = get_package_share_directory('nif_mission_manager_nodes')
    waypoint_manager_path = get_package_share_directory('nif_waypoint_manager_nodes')

    dot = graphviz.Digraph(comment='Mission Tree')   

    pit_wpt = os.path.join(waypoint_manager_path, 'maps',TRACK_NAME,'pit_lane.csv',)
    raceline = os.path.join(waypoint_manager_path, 'maps',TRACK_NAME,'race_line.csv',)

    # raceline = os.path.join(waypoint_manager_path, 'inputs/traj_ltpl_cl',TRACK_NAME,'traj_race_cl.csv',)
    graph = os.path.join(waypoint_manager_path, 'inputs/track_offline_graphs',TRACK_NAME,'stored_graph.pckl')

    mission_yaml_file = os.path.join(mission_manager_path, 'config', TRANSITION_FILE)
    with open(mission_yaml_file) as f:
        mission_yaml = yaml.safe_load(f)

    for i, mission_code_block in enumerate(mission_yaml.get("missions")):
        if mission_code_block.get("active"):
            r = random.random()
            b = random.random()
            g = random.random()
            
            dot.node(str(mission_code_block.get("mission_code")), get_mission_label(mission_code_block))
            if mission_code_block.get("fallback") and mission_code_block.get("fallback").get("active"):
                dot.edge(str(mission_code_block.get("mission_code")), str(mission_code_block.get("fallback").get("mission_code")), constraint='false')

            if mission_code_block.get("activation_area") and  mission_code_block.get("activation_area").get('active'):
                bboxes = mission_code_block.get("activation_area").get("bboxes")
                for box in bboxes:
                    print(box)

                    box = patches.Rectangle((box[0], box[1]), # x_min, y_min
                                            box[2] - box[0], # x-wise width
                                            box[3] - box[1], # y-wise height
                                            linewidth=1, edgecolor=(r, g, b), fc=(r, g, b, 0.3), label='MISSION BOX: ' + str(mission_code_block.get("mission_code")))
                    ax.add_patch(box)


    print(pit_wpt)
    print(raceline)

    WPTFileVisualizer(pit_wpt, "pit-in-entire")
    WPTFileVisualizer(raceline, "race-line")

    dot.view()

    s = graphviz.Digraph('structs', filename='structs_revisited.gv',
                     node_attr={'shape': 'record'})

    s.node('struct1', '<f0> left|<f1> middle|<f2> right')
    s.node('struct2', '<f0> one|<f1> two')
    s.node('struct3', r'hello\nworld |{ b |{c|<here> d|e}| f}| g | h')

    s.edges([('struct1:f1', 'struct2:f0'), ('struct1:f2', 'struct3:here')])

    s.view()
    # dot.render("missions_graph", view=True)
    # RaceLineFileVisualizer(raceline, "race-line")
    # GraphVisualizer(graph, "graph_node")
    plt.axis('equal')
    plt.grid()
    plt.show()

