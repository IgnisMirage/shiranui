import os

from launch import LaunchDescription
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    package_name = 'stanley_planner'

    pkg_share_dir = FindPackageShare(package=package_name).find(package_name)
    params_file = os.path.join(pkg_share_dir, 'config', 'stanley_params.yaml')

    # Stanleyプランナーノード
    stanley_planner_node = Node(
        package=package_name,
        executable='stanley_planner',
        name='stanley_planner',
        parameters=[params_file],
        remappings=[
            ('cmd_vel', 'auto_cmd_vel')  # cmd_velをauto_cmd_velにリマップ
        ]
    )

    nodes = [
        stanley_planner_node,
    ]

    return LaunchDescription(nodes)
