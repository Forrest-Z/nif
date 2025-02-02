import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration
from launch.substitutions import ThisLaunchFileDir
from launch_ros.actions import Node

def generate_launch_description():
    config = (
        os.path.join(
            get_package_share_directory("nif_localization_gtsam_nodes"),
            "config",
            "state_estimator.yaml",
        ),
        )
    # config = LaunchConfiguration(
    #     'params',
    #     default=[ThisLaunchFileDir(), '/../config/state_estimator.yaml'])

    ns = ""
    return LaunchDescription(
        [
            Node(
                package="nif_localization_gtsam_nodes",
                executable="state_estimator_node",
                name="state_estimator_node",
                output="screen",
                emulate_tty=True,
                namespace=ns,
                parameters=[config],
                remappings=[
                    ("ins", "novatel_bottom/inspva"),
                    ("gps", "novatel_bottom/fix"),
                    ("imu_novatel", "novatel_bottom/raw_imu"),
                    ("imu", "novatel_bottom/imu/data"),
                ]
            ),
        ]
    )