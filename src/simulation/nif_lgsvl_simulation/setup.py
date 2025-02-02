from setuptools import setup
import os
from glob import glob

package_name = 'nif_lgsvl_simulation'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob('launch/*.launch.py')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='sw',
    maintainer_email='seungwook1024@kaist.ac.kr',
    description='TODO: Package description',
    license='TODO: License declaration',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'controller.py = nif_lgsvl_simulation.controller:main',
            'publisher_member_function.py = nif_lgsvl_simulation.publisher_member_function:main',
            'subscriber_member_function.py = nif_lgsvl_simulation.subscriber_member_function:main',
        ],
    },
)
