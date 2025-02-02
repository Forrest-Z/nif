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

    global_params_file = None

    if track == LOR:
        global_params_file = 'params_LOR.global.yaml'
    elif track == IMS:
        global_params_file = 'params_IMS.global.yaml'
    else:
        raise RuntimeError("ERROR: invalid track provided: {}".format(track))

    nif_global_parameters_file = os.path.join(
        get_package_share_directory('nif_launch'),
        'config',
        'deploy',
        global_params_file
    )

    global_parameters_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            get_package_share_directory('nif_common_nodes') + '/launch/parameters.launch.py'
        ),
        launch_arguments={
            'nif_global_parameters_file': nif_global_parameters_file
        }.items()
    )

    system_status_manager_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            get_package_share_directory('nif_system_status_manager_nodes') + '/launch/deploy.launch.py'
        ),
    )

    safety_node = Node(
        package='bvs_safety',
        executable='safety_executive_node',
        name='safety_executive_node',
        output='screen'
    )

    robot_description_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            get_package_share_directory('av21_description') + '/launch/deploy.launch.py'
        )
    )

    return LaunchDescription([
        global_parameters_launch,
        system_status_manager_launch,
        safety_node,
        robot_description_launch,
    ])
