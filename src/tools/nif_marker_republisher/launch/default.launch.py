from ament_index_python import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition
import os

def generate_launch_description():

    ac_sim_client_node = Node(
                package='nif_ac_simulation',
                executable='client.py',
                output='screen',
                # namespace='nif',
                # parameters=[config]
    )
    return LaunchDescription(
        [
            ac_sim_client_node
        ])
