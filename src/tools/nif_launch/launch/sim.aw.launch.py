# Copyright 2020-2021, The Autoware Foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Launch file for IAC vehicle."""

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
LG_SVL = 2
track = LG_SVL

def get_share_file(package_name, file_name):
    return os.path.join(get_package_share_directory(package_name), file_name)


def generate_launch_description():
    """Launch all packages for the vehicle in IAC."""
    ssc_interface_param_file = get_share_file(
        package_name='ssc_interface', file_name='param/defaults.param.yaml'
    )

    dbc_file_path = get_share_file(
        package_name='raptor_dbw_can', file_name='launch/CAN1_INDY_V4.dbc'
    )

    ssc_interface_param = DeclareLaunchArgument(
        'ssc_interface_param_file',
        default_value=ssc_interface_param_file,
        description='Path to config file for ssc_interface'
    )

    ssc_interface = Node(
        package='ssc_interface',
        name='ssc_interface_node',
        executable='ssc_interface_node_exe',
        namespace='vehicle',
        output='screen',
        parameters=[LaunchConfiguration('ssc_interface_param_file')],
        remappings=[
            ('gear_select', '/ssc/gear_select'),
            ('arbitrated_speed_commands', '/ssc/arbitrated_speed_commands'),
            ('arbitrated_steering_commands', '/ssc/arbitrated_steering_commands'),
            ('turn_signal_command', '/ssc/turn_signal_command'),
            ('dbw_enabled_feedback', '/ssc/dbw_enabled_fedback'),
            ('gear_feedback', '/ssc/gear_feedback'),
            ('velocity_accel_cov', '/ssc/velocity_accel_cov'),
            ('steering_feedback', '/ssc/steering_feedback'),
            ('vehicle_kinematic_state_cog', '/vehicle/vehicle_kinematic_state'),
            ('state_report_out', '/vehicle/vehicle_state_report'),
            ('state_command', '/vehicle/vehicle_state_command'),

            ('accelerator_pedal_cmd', '/raptor_dbw_interface/accelerator_pedal_cmd'),
            ('brake_cmd', '/raptor_dbw_interface/brake_cmd'),
            ('steering_cmd', '/raptor_dbw_interface/steering_cmd'),
            ('gear_cmd', '/raptor_dbw_interface/gear_cmd'),
            ('ct_status', '/raptor_dbw_interface/ct_report'),

            ('/vehicle/misc_report', '/raptor_dbw_interface/misc_report_do'),
            ('/vehicle/pt_report', '/raptor_dbw_interface/pt_report'),
            ('/vehicle/accelerator_report', '/raptor_dbw_interface/accelerator_report'),
            ('/vehicle/brake_pressure_report', '/raptor_dbw_interface/brake_pressure_report'),
            ('/vehicle/steer_report', '/raptor_dbw_interface/steer_report'),
            ('/vehicle/rc_to_ct_info', '/raptor_dbw_interface/rc_to_ct'),
            ('/vehicle/flag_summary', '/raptor_dbw_interface/flag_summary'),
            ('/vehicle/accelerator_cmd_pt', '/accelerator_cmd_pt'),
            ('/vehicle/brake_cmd_pt', '/brake_cmd_pt'),
            ('/vehicle/gear_cmd_pt', '/gear_cmd_pt')
        ]
    )

    socketcan_receiver_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            get_share_file(
                package_name='ros2_socketcan',
                file_name='launch/socket_can_receiver.launch.py'
            )
        ]),
        launch_arguments={'interface': 'can2'}.items()
    )

    socketcan_sender_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            get_share_file(
                package_name='ros2_socketcan',
                file_name='launch/socket_can_sender.launch.py'
            )
        ]),
        launch_arguments={'interface': 'can2'}.items()
    )

    raptor_node = Node(
        package='raptor_dbw_can',
        executable='raptor_dbw_can_node',
        output='screen',
        namespace='raptor_dbw_interface',
        parameters=[
            {'dbw_dbc_file': dbc_file_path}
        ],
        remappings=[
            ('/raptor_dbw_interface/can_rx', '/from_can_bus'),
            ('/raptor_dbw_interface/can_tx', '/to_can_bus'),
        ],
    )

    telemetry_node = Node(
        package='telemetry',
        executable='telemetry',
        output='screen',
    )

    # Localization
    nif_localization_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            get_package_share_directory('nif_localization_nodes') + '/launch/deploy.launch.py'
        ),
    )

    nif_aw_localization_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            get_package_share_directory('nif_aw_localization_nodes') + '/launch/deploy.launch.py'
        ),
    )
    
    nif_points_preprocessor_launch_bg = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            get_package_share_directory('nif_points_preprocessor_nodes') + '/launch/deploy.launch.py'
        ),
    )

# NIF CONTROL STACK ###############################################

    nif_csl_param_file = get_share_file(
        package_name='nif_control_safety_layer_nodes', file_name='config/deploy.yaml'
    )

    nif_csl_param = DeclareLaunchArgument(
        'control_safety_layer_param_file',
        default_value=nif_csl_param_file,
        description='Path to config file for csl'
    )

    nif_csl_node = Node(
        package='nif_control_safety_layer_nodes',
        executable='nif_control_safety_layer_nodes_exe',
        output='screen',
        parameters=[LaunchConfiguration('control_safety_layer_param_file')],
        remappings=[
            ('in_control_cmd', '/control_pool/control_cmd'),
            ('in_override_control_cmd', '/control_pool/override_cmd'),
            ('in_perception_steering', '/wall_following_steering_cmd'),
            ('out_control_cmd', '/control_safety_layer/out/control_cmd'),
            ('out_steering_control_cmd', '/joystick/steering_cmd'),
            ('out_accelerator_control_cmd', '/joystick/accelerator_cmd'),
            ('out_braking_control_cmd', '/joystick/brake_cmd'),
            ('out_gear_control_cmd', '/joystick/gear_cmd'),
            ('out_desired_acceleration_cmd', '/control_safety_layer/out/desired_accel'),
        ]
    )

    nif_velocity_planning_node = Node(
        package='nif_velocity_planning_node',
        executable='nif_velocity_planning_node_exe',
        output='screen',
        remappings=[
            ('out_desired_velocity', 'velocity_planner/des_vel'),
            ('in_reference_path', 'planning/graph/path_global'),
            # ('in_reference_path', 'planning/path_global'),
            ('in_ego_odometry', 'aw_localization/ekf/odom'),
            ('in_wheel_speed_report', 'raptor_dbw_interface/wheel_speed_report'),
            ('in_imu_data', 'novatel_bottom/imu/data'),
            ('in_steering_report', 'raptor_dbw_interface/steering_report'),
            ('in_control_error', 'control_joint_lqr/lqr_error')
        ],
        parameters=[{
                'odometry_timeout_sec' : 0.5,
                'path_timeout_sec' : 1.0,
            }]
    )

    lqr_joint_config_file = get_share_file(
        package_name='nif_control_joint_lqr_nodes', file_name='config/lqr/lqr_params.sim.yaml'
    )

    nif_joint_lqr_rosparams_file = get_share_file(
        package_name='nif_control_joint_lqr_nodes', file_name='config/deploy.params.yaml'
    )

    nif_joint_lqr_param = DeclareLaunchArgument(
        'control_joint_lqr_params_file',
        default_value=nif_joint_lqr_rosparams_file,
        description='Path to config file for nif_control_lqr'
    )

    nif_joint_lqr_control_node = Node(
        package='nif_control_joint_lqr_nodes',
        executable='nif_control_joint_lqr_nodes_exe',
        parameters=[
            LaunchConfiguration('control_joint_lqr_params_file'),
            {
                'lqr_config_file': lqr_joint_config_file,
                'odometry_timeout_sec' : 0.5,
                'path_timeout_sec' : 1.0,
                'use_tire_velocity' : True,
                # 'max_steering_angle_deg': 20.0,
                'pure_pursuit_min_dist_m' : 8.0,
                'pure_pursuit_max_dist_m' : 16.0,
                'steering_max_ddeg_dt' : 3.0

            }
        ],
        output={
            'stdout': 'screen',
            'stderr': 'screen',
        },
        remappings=[
            ('in_control_cmd_prev', '/control_safety_layer/out/control_cmd'),
            ('out_control_cmd', '/control_pool/control_cmd'),
            ('in_reference_path', '/planning/graph/path_global'),
        ]
    )

    nif_accel_control_param = DeclareLaunchArgument(
        'accel_control_param',
        default_value=get_share_file(package_name='nif_accel_control_nodes', file_name='config/params.yaml'
        ),
        description='Path to config file for nif_accel_control_nodes'
    )

    nif_accel_control_node = Node(
        package='nif_accel_control_nodes',
        executable='nif_accel_control_nodes_exe',
        output='screen',
        parameters=[
            LaunchConfiguration('accel_control_param')
        ]
    )

# NIF LQR + CSL END ###############################################

    global_params_file = None

    if track == LOR:
        global_params_file = 'params_LOR.sim.global.yaml'
    elif track == IMS:
        global_params_file = 'params_IMS.sim.global.yaml'
    elif track == LG_SVL:
        global_params_file = 'params_IMS.sim.global.yaml'
    else:
        raise RuntimeError("ERROR: invalid track provided: {}".format(track))

    nif_global_param = os.path.join(
        get_package_share_directory('nif_launch'),
        'config',
        global_params_file
    )

    nif_global_param = DeclareLaunchArgument(
        'nif_global_parameters_file',
        default_value=nif_global_param,
        description='Path to config file for global parameters'
    )

    nif_global_param_node = Node(
        package='nif_common_nodes',
        executable='nif_global_parameters_exe',
        name='global_parameters_node',
        parameters=[
            LaunchConfiguration('nif_global_parameters_file')
        ]
    )

    nif_system_status_manager_node = Node(
        package='nif_system_status_manager_nodes',
        executable='nif_system_status_manager_nodes_exe',
        remappings=[
            ('in_joystick_cmd', '/joystick/command'),
            ('in_novatel_bestpos', '/novatel_bottom/bestpos'),
            ('in_novatel_insstdev', '/novatel_bottom/insstdev'),
            ('in_localization_status', '/aw_localization/ekf/status'),
            ('in_mission_status', '/system/mission'),
            ('out_system_status', '/system/status'),
        ],
        parameters=[{
            # 'lat_autonomy_enabled': True,
            # 'long_autonomy_enabled': True,
            'safeloc.threshold_stop' : 40.0,
            'safeloc.threshold_slow_down' : 25.0,
        }]
    )

    robot_description_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            get_package_share_directory('av21_description') + '/launch/deploy.launch.py'
        )
    )

### NIF WAYPOINT MANAGER #############################

    wpt_config_file_lor = (
        os.path.join(
            get_package_share_directory("nif_waypoint_manager_nodes"),
            "config",
            "lor.yaml",
        ),
    )

    wpt_config_file_ims = (
        os.path.join(
            get_package_share_directory("nif_waypoint_manager_nodes"),
            "config",
            "ims.yaml",
        ),
    )

    wpt_config_file_svl = (
        os.path.join(
            get_package_share_directory("nif_waypoint_manager_nodes"),
            "config",
            "svl.yaml",
        ),
    )

    config_file = None

    if track == LOR:
        config_file = wpt_config_file_lor
    elif track == IMS:
        config_file = wpt_config_file_ims
    elif track == LG_SVL:
        config_file = wpt_config_file_svl
    else:
        raise RuntimeError("ERROR: invalid track provided: {}".format(track))

    nif_wpt_param = DeclareLaunchArgument(
        'nif_waypoint_manager_param_file',
        default_value=config_file,
        description='Path to config file for waypoint manager'
    )

    nif_waypoint_manager_node = Node(
        package='nif_waypoint_manager_nodes',
        executable='nif_waypoint_manager_nodes_exe',
        output='screen',
        parameters=[
            LaunchConfiguration('nif_waypoint_manager_param_file')
        ],
        remappings=[
            # ('topic_ego_odometry', '/bvs_localization/ltp_odom'),
            ('topic_ego_odometry', 'aw_localization/ekf/odom'),
            ('wpt_manager/maptrack_path/global', '/planning/path_global'),
            ('wpt_manager/maptrack_path/body', '/planning/path_body')
        ]
    )

### NIF WAYPOINT MANAGER END #############################

    nif_multilayer_planning_node = Node(
        package='nif_multilayer_planning_nodes',
        executable='nif_multilayer_planning_nodes_exe',
        output={
            'stdout': 'screen',
            'stderr': 'screen',
        },
        remappings={
            ('out_local_maptrack_inglobal', '/planning/graph/path_global'),
            ('in_ego_odometry', 'aw_localization/ekf/odom'),
            ('in_system_status', '/system/status')
        }
    )

    lgsvl_simulation_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            get_share_file("nif_lgsvl_simulation", 'launch/default.launch.py')
        )
    )

### NIF MULTILAYER PLANNER END #############################

    return LaunchDescription([
        ssc_interface_param,
        nif_global_param,
        nif_csl_param,
        nif_wpt_param,
        nif_joint_lqr_param,
        nif_accel_control_param,

        # ssc_interface,
        # socketcan_receiver_launch,
        # socketcan_sender_launch,
        # raptor_node,
        telemetry_node,

        nif_global_param_node,
        nif_system_status_manager_node,
        nif_csl_node,
        nif_localization_launch,
        nif_aw_localization_launch,
        nif_points_preprocessor_launch_bg,
        nif_waypoint_manager_node,
        robot_description_launch,
        nif_multilayer_planning_node,
        nif_velocity_planning_node,
        nif_joint_lqr_control_node,
        nif_accel_control_node,
        lgsvl_simulation_launch
    ])