'''
@file   subscriber_member_function.py
@auther USRG UGS, Seungwook Lee
@date   2021-08-11
@brief  subscriber node for lgsvl simulation with topic names matched to the real car
'''
import numpy as np
import rclpy
import csv
import pandas as pd

from nav_msgs.msg import Odometry
from sensor_msgs.msg import CompressedImage, Imu, PointCloud2, NavSatFix
from lgsvl_msgs.msg import VehicleOdometry, Detection2DArray
from novatel_oem7_msgs.msg import INSPVA, BESTPOS, INSSTDEV, InertialSolutionStatus
from geometry_msgs.msg import PoseStamped, TransformStamped, Vector3, Quaternion
from raptor_dbw_msgs.msg import WheelSpeedReport

import math
import tf2_py
import tf2_ros
from tf2_ros.transform_broadcaster import TransformBroadcaster

from pymap3d.ecef import ecef2geodetic
from pymap3d.ned import ned2geodetic, geodetic2ned, geodetic2enu

from nifpy_common_nodes.base_node import BaseNode
from nif_lgsvl_simulation.quaternion_euler import Quaternion_Euler


class LGSVLSubscriberNode(BaseNode):
    def __init__(self):
        super().__init__('lgsvl_subscriber')
        self.msgtype = []
        self.namespace = ''
        self.node_namespace = self.get_namespace()
        # self.sub_gps_odometry = self.create_subscription(Odometry, '/simulator/car1/sensor/gps/odometry', self.callback, 10)
        # self.sub_imu = self.create_subscription(Imu, '/simulator/car1/sensor/imu', self.callback2, 10)
        # self.sub_radar= self.create_subscription(Odometry, '/simulator/car1/sensor/radar', self.callback, 10)

        # self.client.wait_for_service(10)
        # if self.client.service_is_ready():
        #     # Define references in global params file
        #     self.use_enu   = bool(self.get_global_parameter('coordinates.use_enu'))
        #     self.sim_ref_x = float(self.get_global_parameter('coordinates.sim_ref_x'))
        #     self.sim_ref_y = float(self.get_global_parameter('coordinates.sim_ref_y'))
        #     self.sim_ref_z = float(self.get_global_parameter('coordinates.sim_ref_z'))
        #     self.loc_ref_x = float(self.get_global_parameter('coordinates.loc_ref_x'))
        #     self.loc_ref_y = float(self.get_global_parameter('coordinates.loc_ref_y'))
        #     self.loc_ref_z = float(self.get_global_parameter('coordinates.loc_ref_z'))
        # else:
        #     raise RuntimeError("Can't connect to global_parameters_node.")

        self.use_enu   = False
        self.sim_ref_x = 0.0 # -1.727
        self.sim_ref_y = 0.0 # 21.37
        self.sim_ref_z = 0.0 # 0.119
        self.loc_ref_x = 0.0 # -314.766
        self.loc_ref_y = 0.0 # 24.387
        self.loc_ref_z = 0.0 # -0.0817

        self.heading_deg = 0.0
        self.gps_top_counter = 0
        self.gps_bottom_counter = 0

        # Camera subscriptions
        self.sub_camera_front_left = self.create_subscription(CompressedImage, self.namespace + '/sensor/camera_front_left', self.callback_camera_front_left, rclpy.qos.qos_profile_sensor_data)
        self.sub_camera_front_right = self.create_subscription(CompressedImage, self.namespace + '/sensor/camera_front_right', self.callback_camera_front_right, rclpy.qos.qos_profile_sensor_data)
        self.sub_camera_rear_left = self.create_subscription(CompressedImage, self.namespace + '/sensor/camera_rear_left', self.callback_camera_rear_left, rclpy.qos.qos_profile_sensor_data)
        self.sub_camera_rear_right = self.create_subscription(CompressedImage, self.namespace + '/sensor/camera_rear_right', self.callback_camera_rear_right, rclpy.qos.qos_profile_sensor_data)
        self.sub_camera_front_1 = self.create_subscription(CompressedImage, self.namespace + '/sensor/camera_front_1', self.callback_camera_front_1, rclpy.qos.qos_profile_sensor_data)
        self.sub_camera_front_2 = self.create_subscription(CompressedImage, self.namespace + '/sensor/camera_front_2', self.callback_camera_front_2, rclpy.qos.qos_profile_sensor_data)

        # 2D ground truth sensor subscriptions
        self.sub_2D_front_left = self.create_subscription(Detection2DArray, self.namespace + '/sensor/a2D_front_left', self.callback_2D_front_left, rclpy.qos.qos_profile_sensor_data)
        self.sub_2D_front_right = self.create_subscription(Detection2DArray, self.namespace + '/sensor/a2D_front_right', self.callback_2D_front_right, rclpy.qos.qos_profile_sensor_data)
        self.sub_2D_rear_left = self.create_subscription(Detection2DArray, self.namespace + '/sensor/a2D_rear_left', self.callback_2D_rear_left, rclpy.qos.qos_profile_sensor_data)
        self.sub_2D_rear_right = self.create_subscription(Detection2DArray, self.namespace + '/sensor/a2D_rear_right', self.callback_2D_rear_right, rclpy.qos.qos_profile_sensor_data)
        self.sub_2D_front_1 = self.create_subscription(Detection2DArray, self.namespace + '/sensor/a2D_front_1', self.callback_2D_front_1, rclpy.qos.qos_profile_sensor_data)
        self.sub_2D_front_2 = self.create_subscription(Detection2DArray, self.namespace + '/sensor/a2D_front_2', self.callback_2D_front_2, rclpy.qos.qos_profile_sensor_data)

        # Lidar subscriptions
        self.sub_laser_meter_flash_a = self.create_subscription(PointCloud2, self.namespace + '/sensor/laser_meter_flash_a', self.callback_laser_meter_flash_a, rclpy.qos.qos_profile_sensor_data)
        self.sub_laser_meter_flash_b = self.create_subscription(PointCloud2, self.namespace + '/sensor/laser_meter_flash_b', self.callback_laser_meter_flash_b, rclpy.qos.qos_profile_sensor_data)
        self.sub_laser_meter_flash_c = self.create_subscription(PointCloud2, self.namespace + '/sensor/laser_meter_flash_c', self.callback_laser_meter_flash_c, rclpy.qos.qos_profile_sensor_data)

        # IMU subscriptions
        self.sub_imu_top = self.create_subscription(Imu, self.namespace + '/novatel_top/rawimux', self.callback_imu_top, rclpy.qos.qos_profile_sensor_data)
        self.sub_imu_bottom = self.create_subscription(Imu, self.namespace + '/novatel_bottom/rawimux', self.callback_imu_bottom, rclpy.qos.qos_profile_sensor_data)

        # Vehicle odometry subsciptions (includes front/rear wheel angles and velocity)
        self.sub_vehicleodometry = self.create_subscription(VehicleOdometry, self.namespace + '/sensor/odometry', self.callback_vehicleodometry, rclpy.qos.qos_profile_sensor_data)
        self.pub_wheel_speed = self.create_publisher(WheelSpeedReport, self.namespace + '/raptor_dbw_interface/wheel_speed_report', 20) # rclpy.qos.qos_profile_sensor_data)
        # GPS subscriptions
        # self.sub_gps_top = self.create_subscription(NavSatFix, self.namespace + '/novatel_top/fix', self.callback_gps_top, rclpy.qos.qos_profile_sensor_data)
        # self.sub_gps_bottom = self.create_subscription(NavSatFix, self.namespace + '/novatel_bottom/fix', self.callback_gps_bottom, rclpy.qos.qos_profile_sensor_data)

        # Convert to Novatel specific topics
        self.pub_gps_inspva_top = self.create_publisher(INSPVA, self.namespace + '/novatel_top/inspva', rclpy.qos.qos_profile_sensor_data)
        self.pub_gps_inspva_bottom = self.create_publisher(INSPVA, self.namespace + '/novatel_bottom/inspva', rclpy.qos.qos_profile_sensor_data)

        self.pub_gps_insstdev_top = self.create_publisher(INSSTDEV, self.namespace + '/novatel_top/insstdev', 10) #rclpy.qos.qos_profile_sensor_data)
        self.pub_gps_insstdev_bottom = self.create_publisher(INSSTDEV, self.namespace + '/novatel_bottom/insstdev', 10) #rclpy.qos.qos_profile_sensor_data)

        self.pub_gps_bestpos_top = self.create_publisher(BESTPOS, self.namespace + '/novatel_top/bestpos', 10) #rclpy.qos.qos_profile_sensor_data)
        self.pub_gps_bestpos_bottom = self.create_publisher(BESTPOS, self.namespace + '/novatel_bottom/bestpos', 10) #rclpy.qos.qos_profile_sensor_data)


        # Vehicle ground truth state
        self.sub_ground_truth_state = self.create_subscription(Odometry, '/sensor/gps_ground_truth', self.callback_ground_truth_state, rclpy.qos.qos_profile_sensor_data)
        self.pub_ground_truth_state = self.create_publisher(Odometry, '/sensor/odom_ground_truth', rclpy.qos.qos_profile_sensor_data)
        self.pub_odom_converted = self.create_publisher(Odometry, '/sensor/odom_converted', rclpy.qos.qos_profile_sensor_data)
        self._tf_publisher = TransformBroadcaster(self)
    # def callback(self, msg):
    # self.get_logger().info('Subscribed GPS ODOM: {}'.format(msg.pose.pose.position.x))

    def callback_camera_front_left(self, msg):
        pass
        # self.get_logger().info('Subscribed camera_front_left')

    def callback_camera_front_right(self, msg):
        pass
        # self.get_logger().info('Subscribed camera_front_right')

    def callback_camera_rear_left(self, msg):
        pass
        # self.get_logger().info('Subscribed camera_rear_left')

    def callback_camera_rear_right(self, msg):
        pass
        # self.get_logger().info('Subscribed camera_rear_right')

    def callback_camera_front_1(self, msg):
        pass
        # self.get_logger().info('Subscribed camera_front_1')

    def callback_camera_front_2(self, msg):
        pass
        # self.get_logger().info('Subscribed camera_front_2')

    def callback_2D_front_left(self, msg):
        pass
        # self.get_logger().info('Subscribed 2D_front_left ' + str(msg))

    def callback_2D_front_right(self, msg):
        pass
        # self.get_logger().info('Subscribed 2D_front_right ' + str(msg))

    def callback_2D_rear_left(self, msg):
        pass
        # self.get_logger().info('Subscribed 2D_rear_left '+ str(msg))

    def callback_2D_rear_right(self, msg):
        pass
        # self.get_logger().info('Subscribed 2D_rear_right '+ str(msg))

    def callback_2D_front_1(self, msg):
        pass
        # self.get_logger().info('Subscribed 2D_front_1 '+ str(msg))

    def callback_2D_front_2(self, msg):
        pass
        # self.get_logger().info('Subscribed 2D_front_2 '+ str(msg))

    def callback_laser_meter_flash_a(self, msg):
        pass
        # self.get_logger().info('Subscribed laser_meter_flash_a')

    def callback_laser_meter_flash_b(self, msg):
        pass
        # self.get_logger().info('Subscribed laser_meter_flash_b')

    def callback_laser_meter_flash_c(self, msg):
        pass
        # self.get_logger().info('Subscribed laser_meter_flash_c')

    def callback_imu_top(self, msg):
        pass
        # self.get_logger().info('Subscribed imu_top')

    def callback_imu_bottom(self, msg):
        pass
        # self.get_logger().info('Subscribed imu_bottom')

    def callback_vehicleodometry(self, msg : VehicleOdometry):
        # wheel_speed_msg = WheelSpeedReport()
        # wheel_speed_msg.header = msg.header
        # vel_kph = msg.velocity * 3.6 # / 3.0 # Correction factor (completely empirical)
        # wheel_speed_msg.front_left = vel_kph
        # wheel_speed_msg.front_right = vel_kph
        # wheel_speed_msg.rear_left = vel_kph
        # wheel_speed_msg.rear_right = vel_kph

        # self.pub_wheel_speed.publish(wheel_speed_msg)
        # self.get_logger().info('Subscribed vehicleodometry')
        pass


    def callback_gps_top(self, msg):
        noise = [self.random_noise_position(), self.random_noise_heading()]
        self.gps_top_counter += 1

        inspva_msg = self.convert_fix_to_inspva(msg, noise)
        bestpos_msg = self.convert_fix_to_bestbos(msg, noise)
        insstdev_msg = self.convert_fix_to_insstdev(msg, noise)
        self.pub_gps_inspva_top.publish(inspva_msg)
        if (self.gps_top_counter % 3) == 0:
            self.pub_gps_bestpos_top.publish(bestpos_msg)
        self.pub_gps_insstdev_top.publish(insstdev_msg)

    def callback_gps_bottom(self, msg):
        noise = [self.random_noise_position(), self.random_noise_heading()]
        self.gps_bottom_counter += 1   

        inspva_msg = self.convert_fix_to_inspva(msg, noise)
        bestpos_msg = self.convert_fix_to_bestbos(msg, noise)
        insstdev_msg = self.convert_fix_to_insstdev(msg, noise)
        self.pub_gps_inspva_bottom.publish(inspva_msg)
        if (self.gps_bottom_counter % 3) == 0:
            self.pub_gps_bestpos_bottom.publish(bestpos_msg)
        self.pub_gps_insstdev_bottom.publish(insstdev_msg)

        # self.get_logger().info('Subscribed gps_bottom latitude: ' + str(msg.latitude) + ' altitude: '+ str(msg.altitude))
        # WPT_CSV_PATH = "./src/subtest/wpt_data/wpt_from_GPS2.csv"
        # csv_data = pd.read_csv(WPT_CSV_PATH, sep=',', header=None)
        # wpts_x = csv_data.values[:,0]
        # wpts_y = csv_data.values[:,1]
        # f = open('./src/subtest/wpt_data/wpt_from_GPS.csv','a',newline='')
        # wr = csv.writer(f)
        # wr.writerow([str(msg.longitude),str(msg.latitude)])
        # f.close

    def convert_fix_to_inspva(self, fix : NavSatFix, noise):
        inspva_msg = INSPVA()
        status = InertialSolutionStatus()
        inspva_msg.header = fix.header
        inspva_msg.latitude = fix.latitude + noise[0] * 0.000001
        inspva_msg.longitude = fix.longitude + noise[0] * 0.000001
        inspva_msg.height = fix.altitude + noise[0] * 0.000001
        inspva_msg.azimuth = - self.heading_deg + noise[1]
        inspva_msg.status.status = 3
        # inspva_msg.azimuth = self.heading_deg + noise[1]

        return inspva_msg

    def convert_fix_to_bestbos(self, fix : NavSatFix, noise):
        bestpos_msg = BESTPOS()
        bestpos_msg.header = fix.header
        bestpos_msg.lat = fix.latitude + noise[0] * 0.000001
        bestpos_msg.lon = fix.longitude + noise[0] * 0.000001
        bestpos_msg.hgt = fix.altitude + noise[0] * 0.000001
        return bestpos_msg

    def convert_fix_to_insstdev(self, fix : NavSatFix, noise):
        insstdev_msg = INSSTDEV()
        insstdev_msg.header = fix.header
        insstdev_msg.latitude_stdev =  abs(noise[0])
        insstdev_msg.longitude_stdev = abs(noise[0])
        insstdev_msg.azimuth_stdev = abs(noise[1])
        
        return insstdev_msg

    def random_noise_heading(self) -> float:
        return np.random.normal(.0, 0.025)

    def random_noise_position(self) -> float:
        return np.random.normal(.0, 0.25)

    def callback_ground_truth_state(self, msg: Odometry):
        '''
        simulator side:
            - ignore map origin: true
        from lgsvl_simulator(ignore map origin):
            t:(-1.727, 21.37, 0.119) q: (0,0,-0.7071,0.7071) *SEU* South-East-Up

        ros localization side:
            - ecef_ref_lat: 39.79312996
            - ecef_ref_lon: -86.23524024
            - ecef_ref_hgt: -0.170035537
        from nif_localization_node(config: ecef_ref_lgsvl_sim.yaml, topic: /nif/localization/ego_odom):
            t: (-314.766, 24.387, -0.0817) q: (0,0,0.7071,0.7071) *ENU*
        @todo Find conversion from (0,0,-0.7071,0.7071) to (0,0,0.7071,0.7071)
        @param convert_seu_to either to "enu" or "ned"
        '''

        # FAKE FIX
        ecef_ref_lat = 36.27395157819169   #LVMS_SIM
        ecef_ref_lon = -115.01206308896546 #LVMS_SIM
        ecef_ref_hgt = 603.3212349116802         #LVMS_SIM

        llh = ned2geodetic(msg.pose.pose.position.x, -msg.pose.pose.position.y, 0., 
                           ecef_ref_lat, ecef_ref_lon, 0.)
        fix = NavSatFix()
        fix.header = msg.header
        fix.latitude = llh[0]
        fix.longitude = llh[1]
        fix.altitude = llh[2]
        self.callback_gps_top(fix)
        self.callback_gps_bottom(fix)
        
        odom_converted = Odometry()
        odom_converted.header = msg.header
        odom_converted.child_frame_id = msg.child_frame_id
        odom_converted.pose = msg.pose
        odom_converted.pose.pose.position.x = msg.pose.pose.position.x
        odom_converted.pose.pose.position.y = msg.pose.pose.position.y
        odom_converted.pose.pose.position.z = 0.0  # msg.pose.pose.position.z
        odom_converted.pose.pose.orientation.x = 0.0  # msg.pose.pose.orientation.x
        odom_converted.pose.pose.orientation.y = 0.0  # msg.pose.pose.orientation.y
        odom_converted.pose.pose.orientation.z = msg.pose.pose.orientation.z
        odom_converted.pose.pose.orientation.w = msg.pose.pose.orientation.w
        odom_converted.twist = msg.twist
        self.pub_ground_truth_state.publish(odom_converted)

        quat = Quaternion(x=0.0, y=0.0, z=msg.pose.pose.orientation.z, w=msg.pose.pose.orientation.w)
        qe = Quaternion_Euler(q=quat)
        self.heading_deg = math.degrees(qe.ToEuler().z)

        wheel_speed_msg = WheelSpeedReport()
        wheel_speed_msg.header = msg.header
        vel_kph = msg.twist.twist.linear.x * 3.6
        wheel_speed_msg.front_left = vel_kph
        wheel_speed_msg.front_right = vel_kph
        wheel_speed_msg.rear_left = vel_kph
        wheel_speed_msg.rear_right = vel_kph

        self.pub_wheel_speed.publish(wheel_speed_msg)
        # self.get_logger().info('Subscribed vehicleodometry')

        # options: "enu", "ned"
        convert_seu_to = "enu" if self.use_enu else "ned"
        convert_seu_to = "seu"

        if convert_seu_to == "enu":
            # From SEU to ENU
            temp_x = odom_converted.pose.pose.position.y
            temp_y = -odom_converted.pose.pose.position.x
            temp_z = odom_converted.pose.pose.position.z
            odom_converted.pose.pose.position.x = temp_x
            odom_converted.pose.pose.position.y = temp_y
            odom_converted.pose.pose.position.z = temp_z

            # Calibrate position
            odom_converted.pose.pose.position.x += self.loc_ref_x - self.sim_ref_y
            odom_converted.pose.pose.position.y += self.loc_ref_y + self.sim_ref_x
            odom_converted.pose.pose.position.z += self.loc_ref_z - self.sim_ref_z

            # Quaternion manipulation
            qe = Quaternion_Euler(msg.pose.pose.orientation, None)
            e = qe.ToEuler()
            e_new = Vector3()
            e_new.x = e.y
            e_new.y = -e.x
            e_new.z = e.z
            # e_new.z = e.z + math.pi / 2.0
            qe = Quaternion_Euler(None, e_new)
            odom_converted.pose.pose.orientation = qe.ToQuaternion()

        elif convert_seu_to == "ned":
            # From SEU to NED
            temp_x = -odom_converted.pose.pose.position.x
            temp_y = odom_converted.pose.pose.position.y
            temp_z = -odom_converted.pose.pose.position.z
            odom_converted.pose.pose.position.x = temp_x
            odom_converted.pose.pose.position.y = temp_y
            odom_converted.pose.pose.position.z = temp_z

            # Calibrate position
            odom_converted.pose.pose.position.x += self.loc_ref_x + self.sim_ref_x
            odom_converted.pose.pose.position.y += self.loc_ref_y - self.sim_ref_y
            odom_converted.pose.pose.position.z += self.loc_ref_z + self.sim_ref_z

            # Quaternion manipulation
            qe = Quaternion_Euler(msg.pose.pose.orientation, None)
            e = qe.ToEuler()
            e_new = Vector3()
            e_new.x = -e.x
            e_new.y = e.y
            e_new.z = -e.z
            # e_new.z = e.z + math.pi / 2.0
            qe = Quaternion_Euler(None, e_new)
            odom_converted.pose.pose.orientation = qe.ToQuaternion()

        elif convert_seu_to == "seu":
            pass
        else:
            # Calibrate position
            odom_converted.pose.pose.position.x += self.loc_ref_x - self.sim_ref_x
            odom_converted.pose.pose.position.y += self.loc_ref_y - self.sim_ref_y
            odom_converted.pose.pose.position.z += self.loc_ref_z - self.sim_ref_z

        self.pub_odom_converted.publish(odom_converted)
        self.tf_broadcast(odom_converted)

    def tf_broadcast(self, msg):
        tfs = TransformStamped()
        tfs.header = msg.header
        tfs.child_frame_id = msg.child_frame_id
        tfs.transform.translation.x = msg.pose.pose.position.x
        tfs.transform.translation.y = msg.pose.pose.position.y
        tfs.transform.translation.z = msg.pose.pose.position.z
        tfs.transform.rotation.x = msg.pose.pose.orientation.x
        tfs.transform.rotation.y = msg.pose.pose.orientation.y
        tfs.transform.rotation.z = msg.pose.pose.orientation.z
        tfs.transform.rotation.w = msg.pose.pose.orientation.w
        self._tf_publisher.sendTransform(tfs)

def main(args=None):
    rclpy.init(args=args)
    drive = LGSVLSubscriberNode()
    rclpy.spin(drive)


if __name__ == '__main__':
    main()
