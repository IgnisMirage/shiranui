"""Package setup for zed_f9r."""

from glob import glob
import os

from setuptools import find_packages, setup

package_name = 'zed_f9r'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        (os.path.join('share', package_name), ['package.xml']),
        (os.path.join('share', package_name, 'launch'), glob('launch/*.xml')),
        (os.path.join('share', package_name, 'config'), glob('config/*.yaml')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='yoshida',
    maintainer_email='k-yoshida@industryalpha.net',
    description=('ROS 2 UBX driver for u-blox ZED-F9R using the '
                 'SparkFun sparkfun-ublox-gps library'),
    license='Apache-2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'zed_f9r_node = zed_f9r.zed_f9r_node:main',
        ],
    },
)
