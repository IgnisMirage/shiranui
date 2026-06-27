from launch_ros.actions import Node
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    use_sim_time = LaunchConfiguration("use_sim_time", default="true")
    planner_config = PathJoinSubstitution(
        [FindPackageShare("a_star_planner"), "config", "planner_params.yaml"]
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument("use_sim_time", default_value="true"),
            Node(
                package="a_star_planner",
                executable="a_star_component",
                name="a_star_planner_node",
                output="screen",
                parameters=[planner_config, {"use_sim_time": use_sim_time}],
            ),
        ]
    )
