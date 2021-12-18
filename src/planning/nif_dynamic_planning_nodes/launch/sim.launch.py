import os
from ament_index_python import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
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

def get_share_file(package_name, file_name):
    return os.path.join(get_package_share_directory(package_name), file_name)

def generate_launch_description():
  
    config_file_lor = (
        os.path.join(
            get_package_share_directory("nif_dynamic_planning_nodes"),
            "config",
            "planner_config.yaml",
        ),
    )

    config_file_ims = (
        os.path.join(
            get_package_share_directory("nif_dynamic_planning_nodes"),
            "config",
            "planner_config.yaml",
        ),
    )

    config_file_lvms_sim = (
        os.path.join(
            get_package_share_directory("nif_dynamic_planning_nodes"),
            "config",
            "planner_config.yaml",
        ),
    )

    config_file_lvms = (
        os.path.join(
            get_package_share_directory("nif_dynamic_planning_nodes"),
            "config",
            "planner_config.yaml",
        ),
    )

    maps_path_root = get_package_share_directory("nif_waypoint_manager_nodes")
        
    config_file = None
    if track == LOR:
        config_file = config_file_lor
    elif track == IMS:
        config_file = config_file_ims
    elif track == LVMS:
        config_file = config_file_lvms
    elif track == LVMS_SIM:
        config_file = config_file_lvms_sim
    else:
        raise RuntimeError("ERROR: invalid track provided: {}".format(track))

    param_file_argument = DeclareLaunchArgument(
        'nif_dynamic_planning_param_file',
        default_value=config_file,
        description='Path to config file for dynamic planner'
    )

    prediction_node = Node(
        package='nif_dynamic_planning_nodes',
        executable='nif_dynamic_planning_nodes_exe',
        output='screen',
        parameters=[
            LaunchConfiguration('nif_dynamic_planning_param_file'),
            {
                'maps_path_root': maps_path_root         
            }
        ],
        remappings=[
            ('tracking_output_topic_name', '/tracking/objects'),
            ('prediction_output_topic_name', '/oppo/prediction'),
        ]
    )

    return LaunchDescription([
        param_file_argument,
        prediction_node
    ])
