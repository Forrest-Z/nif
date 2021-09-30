import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument

from launch.substitutions import LaunchConfiguration
from launch.substitutions import ThisLaunchFileDir
from launch_ros.actions import Node

IMS = 0
LOR = 1
LG_SVL = 2

track_id = os.environ.get('TRACK').strip()

if track_id == "IMS":
    track = IMS
elif track_id == "LOR":
    track = LOR
elif track_id == "LG_SVL":
    track = LG_SVL
else:
    raise RuntimeError("ERROR: Invalid track {}".format(track_id))

def generate_launch_description():

    config_file = None

    if track == LOR:
        config_file = 'config_lor.yaml'
    elif track == IMS:
        config_file = 'config_ims.yaml'
    elif track == LG_SVL:
        config_file = 'config_lgsim.yaml'
    else:
        raise RuntimeError("ERROR: invalid track provided: {}".format(track))

    params_file = os.path.join(
            get_package_share_directory("nif_aw_localization_nodes"),
            "config",
            config_file
        )
        
    param_file_launch_arg = DeclareLaunchArgument(
        'nif_aw_localization_param_file',
        default_value=params_file,
        description='nif_aw_localization_param'
    )

    aw_localization_node =  Node(
                package="nif_aw_localization_nodes",
                executable="nif_aw_localization_nodes_exe",
                output={
                    "stderr": "log",
                    "stdout": "screen"
                },
                emulate_tty=True,
                parameters=[
                    LaunchConfiguration('nif_aw_localization_param_file')
                ],
                remappings=[
                    # Current set : Bottom INS Disabled // Top INS Enabled
                    # /novatel_bottom/bestvel is used to back-up solution when novatel_top/inspva heading is not published.  
                    ("in_inspva", "novatel_bottom/inspva"), # NOT USED
                    ("in_top_inspva", "novatel_top/inspva_nouse"), # HEADING
                    ("in_bestpos", "novatel_bottom/bestpos"), # POSE (X,Y)
                    ("in_top_bestpos", "novatel_top/bestpos"), # POSE (X,Y)
                    ("in_imu", "novatel_bottom/imu/data"), # YAW RATE
                    ("in_bestvel", "novatel_bottom/bestvel"), #HEADING BACK UP SOLUTION
                    ("in_insstdev", "novatel_bottom/insstdev"), #INS STANDARD DEVIATION
                    ("in_wheel_speed_report", "raptor_dbw_interface/wheel_speed_report"), # WHEEL SPEED

                    ("out_odometry_ekf_estimated", "/aw_localization/ekf/odom"),
                    ("out_odometry_bestpos", "/aw_localization/ekf/odom_bestpos"),
                    ("out_top_odometry_bestpos", "/aw_localization/ekf/top_bestpos"),
                    ('out_localization_error', '/aw_localization/ekf/error')
                ]
    )

    return LaunchDescription(
        [
            param_file_launch_arg,
            aw_localization_node    
        ])
    

