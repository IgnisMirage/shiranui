
FROM ros:jazzy-ros-base AS base

ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=jazzy

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    python3-colcon-common-extensions \
    python3-pip \
    python3-serial \
    libyaml-cpp-dev \
    libpng-dev \
    libeigen3-dev \
    libomp-dev \
    libpcl-dev \
    can-utils \
    ros-jazzy-ament-lint \
    ros-jazzy-angles \
    ros-jazzy-foxglove-bridge \
    ros-jazzy-joy \
    ros-jazzy-pcl-conversions \
    ros-jazzy-pointcloud-to-laserscan \
    ros-jazzy-tf2-sensor-msgs \
    ros-jazzy-visualization-msgs \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

RUN pip3 install --break-system-packages --no-cache-dir \
    sparkfun-ublox-gps \
    pyserial \
    spidev

SHELL ["/bin/bash", "-c"]

FROM base AS build
COPY src /ros2_ws/src
WORKDIR /ros2_ws

RUN source /opt/ros/jazzy/setup.bash \
  && colcon build \
  && rm -rf log build

FROM base AS deploy

COPY --from=build /ros2_ws/install /ros2_ws/install

WORKDIR /ros2_ws
ENTRYPOINT ["/bin/bash", "-c", "source /opt/ros/jazzy/setup.bash && [ -f /ros2_ws/install/setup.bash ] && source /ros2_ws/install/setup.bash; exec \"$0\" \"$@\""]
CMD ["ros2", "launch", "autonomous_drive", "autonomous_drive.launch.xml"]

FROM ros:jazzy-ros-base AS mqttsn-bridge

WORKDIR /app
COPY mqttsn_bridge/mqttsn_bridge /app/mqttsn_bridge

ENV FASTDDS_BUILTIN_TRANSPORTS=UDPv4
ENV MQTTSN_PORT=1884
CMD ["python3", "-m", "mqttsn_bridge"]

FROM base AS devenv

WORKDIR /ros2_ws
ENTRYPOINT ["/bin/bash", "-c", "source /opt/ros/jazzy/setup.bash && [ -f /ros2_ws/install/setup.bash ] && source /ros2_ws/install/setup.bash; exec \"$0\" \"$@\""]
CMD ["bash"]
