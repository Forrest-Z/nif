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
track = None

# get which track we are at
# track_id = os.environ.get('TRACK').strip()

track_id = "IMS"


if track_id == "IMS":
    track = IMS
elif track_id == "LOR":
    track = LOR
else:
    raise RuntimeError("ERROR: Invalid track {}".format(track_id))


def generate_launch_description():
    ns = ""
    track_subdir = ""

    if track == LOR:
        track_subdir = "lor"
    elif track == IMS:
        track_subdir = "ims"
    else:
        raise RuntimeError("ERROR: invalid track provided: {}".format(track))

    global_map = os.path.join(
        get_package_share_directory("nif_localization_nodes"),
        "map",
        track_subdir,
        "full_map.pcd", #IMS
    )
    outer_geofence_map = os.path.join(
        get_package_share_directory("nif_localization_nodes"),
        "map",
        track_subdir,
        "outer_map.pcd" # IMS
        # "outer_map.pcd", #LOR
    )
    inner_geofence_map = os.path.join(
        get_package_share_directory("nif_localization_nodes"),
        "map",
        track_subdir,
        "inner_map.pcd", #IMS
        # "inner_map.pcd", #LOR
    )
    trajectory_map = os.path.join(
        get_package_share_directory("nif_localization_nodes"),
        "map",
        track_subdir,
        # "have_to_generate.pcd", #IMS
        "trajectory.pcd", #LOR
    )
    # TOOO +++++ INCLUDE WALL DETECTION (UNWIRED) + TOPICS FOR ROSBAGS
    base_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            get_package_share_directory('nif_launch') + '/launch/base.launch.py'
        )
    )


    # TOOO +++++ INCLUDE WALL DETECTION (UNWIRED) + TOPICS FOR ROSBAGS

    localization_node = Node(
        package="nif_localization_nodes",
        executable="nif_localization_nodes_exe",
        output="screen",
        emulate_tty=True,
        namespace=ns,
        parameters=[{
            # global map loader
            'globalmap_file_name' : global_map,
            'trajectory_pcd_file' : trajectory_map,
            'use_trajectory' : True,

            # geofence node
            'outer_geofence_filename': outer_geofence_map,
            'inner_geofence_filename': inner_geofence_map,
            'outer_geofence_bias': -0.5,
            'inner_geofence_bias': 0.0,
            'distance_low_pass_filter': 0.5,

            # resilient localization node
            'thres_for_distance_error_flag' : 1.0,
            'thres_for_distance_to_wall' : 2.0,

            # EKF node
            'use_inspva_heading' : True,
            'bestvel_heading_update_velocity_thres' : 2.0, # unit : km/h
            # 'origin_lat' : 39.809786,
            # 'origin_lon' : -86.235148,
        }],

        remappings=[
            ("in_odometry_ekf_estimated", "/aw_localization/ekf/odom"),
        ]
    )

    return LaunchDescription(
        [
            base_launch, 
            localization_node
        ]
    )




