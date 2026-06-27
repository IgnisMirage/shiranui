from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='mcl_3d',
            executable='distance_field_builder',
            name='distance_field_builder',
            output='screen',
            parameters=[{
                'pcd_file':           '/tmp/map.pcd',
                'map_file_name':      'dist_map.bin',
                'yaml_file_path':     '/tmp/dist_map.yaml',
                'resolution':         5.0,
                'sub_map_resolution': 0.1,
                'map_margin':         1.0,
                'voxel_leaf_size':    0.1,
            }],
        ),
    ])
