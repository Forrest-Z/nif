
  
import os
from ament_index_python import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition
from launch_ros.actions import Node


IMS = 0
LOR = 1
IMS_SIM = 2
LVMS = 3
LVMS_SIM = 4
track = None

# get which track we are at
track_id = os.environ.get('TRACK').strip()

if track_id == "IMS" or track_id == "ims":
    track = IMS
elif track_id == "LOR" or track_id == "lor":
    track = LOR
elif track_id == "IMS_SIM" or track_id == "ims_sim":
    track = IMS_SIM
elif track_id == "LVMS" or track_id == "lvms":
    track = LVMS
elif track_id == "LVMS_SIM" or track_id == "lvms_sim":
    track = LVMS_SIM
else:
    raise RuntimeError("ERROR: Invalid track {}".format(track_id))

def generate_launch_description():
    ns = ""
    track_subdir = ""

    if track == LOR:
        track_subdir = "LOR"
    elif track == IMS:
        track_subdir = "IMS"
    elif track == IMS_SIM:
        track_subdir = "IMS_SIM"
    elif track == LVMS:
        track_subdir = "LVMS"
    elif track == LVMS_SIM:
        track_subdir = "LVMS_SIM"
    else:
        raise RuntimeError("ERROR: invalid track provided: {}".format(track))

    file_path_inner_line = os.path.join(
                get_package_share_directory("nif_waypoint_manager_nodes"),
                "maps",
                track_subdir,
                "trackboundary_left.csv",
            )
    file_path_outer_line = os.path.join(
                get_package_share_directory("nif_waypoint_manager_nodes"),
                "maps",
                track_subdir,
                "trackboundary_right.csv",
            )

    geofence_filter_node = Node(
                package="nif_geofence_filter_nodes",
                executable="nif_geofence_filter_nodes_exe",
                output="screen",
                emulate_tty=True,
                parameters=[{
                    # geofence file path
                    'file_path_inner_line' : file_path_inner_line,
                    'file_path_outer_line' : file_path_outer_line,

                    'distance_filter_active': True,
                    'distance_filter_threshold_m': 0.5,

                    'boundaries_filter_active': True,
                    
                    }],

                remappings=[
                    ("in_perception_array", "/ghost/perception"),
                    ("out_filtered_perception_array", "/ghost/perception/filtered"),
                ]
            )

    return LaunchDescription(
        [
            geofence_filter_node
        ]
    )



