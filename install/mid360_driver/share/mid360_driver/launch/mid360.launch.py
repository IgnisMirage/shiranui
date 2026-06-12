from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    config_file = PathJoinSubstitution([
        FindPackageShare('mid360_driver'),
        'config',
        'mid360_config.yaml',
    ])

    return LaunchDescription([
        DeclareLaunchArgument('config_file', default_value=config_file),
        DeclareLaunchArgument('frame_id', default_value='livox_frame'),
        DeclareLaunchArgument('xfer_format', default_value='0'),
        DeclareLaunchArgument('publish_freq', default_value='10.0'),
        Node(
            package='mid360_driver',
            executable='mid360_driver_node',
            name='mid360_driver_node',
            output='screen',
            parameters=[
                LaunchConfiguration('config_file'),
                {
                    'frame_id': LaunchConfiguration('frame_id'),
                    'xfer_format': LaunchConfiguration('xfer_format'),
                    'publish_freq': LaunchConfiguration('publish_freq'),
                },
            ],
        ),
    ])
