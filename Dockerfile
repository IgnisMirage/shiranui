
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
COPY . /ros2_ws
WORKDIR /ros2_ws

# TEMP: on failure, dump how ndt_omp_node is linked (arm64 CI cannot resolve PCL symbols there)
RUN source /opt/ros/jazzy/setup.bash \
  && (colcon build || ( \
       L=/usr/lib/$(gcc -dumpmachine); \
       echo "=== DIAG link.txt"; cat build/ndt_omp/CMakeFiles/ndt_omp_node.dir/link.txt; \
       echo "=== DIAG flags.make"; cat build/ndt_omp/CMakeFiles/ndt_omp_node.dir/flags.make; \
       echo "=== DIAG libs"; ls -lL $L/libpcl_common.so* $L/libpcl_io.so*; file -L $L/libpcl_common.so; \
       echo "=== DIAG console::print defined in libpcl_common:"; nm -D --defined-only $L/libpcl_common.so | grep -c _ZN3pcl7console5print; \
       echo "=== DIAG undefined in ndt_omp_node.o:"; nm -u build/ndt_omp/CMakeFiles/ndt_omp_node.dir/src/ndt_omp_node.cpp.o | grep -c _ZN3pcl7console5print; \
       echo "=== DIAG relink verbose"; cd build/ndt_omp && bash -c "$(cat CMakeFiles/ndt_omp_node.dir/link.txt) -Wl,--trace" 2>&1 | grep -iE "pcl_common|pcl_io|error" | head -20; \
       echo "=== DIAG pkgs"; dpkg -l | grep -E "libpcl-(common|io)|binutils " ; \
       exit 1)) \
  && rm -rf log build

FROM base AS deploy

COPY --from=build /ros2_ws/install /ros2_ws/install

WORKDIR /ros2_ws
ENTRYPOINT ["/bin/bash", "-c", "source /opt/ros/jazzy/setup.bash && [ -f /ros2_ws/install/setup.bash ] && source /ros2_ws/install/setup.bash; exec \"$0\" \"$@\""]
CMD ["ros2", "launch", "autonomous_drive", "autonomous_drive.launch.xml"]

FROM base AS devenv

WORKDIR /ros2_ws
ENTRYPOINT ["/bin/bash", "-c", "source /opt/ros/jazzy/setup.bash && [ -f /ros2_ws/install/setup.bash ] && source /ros2_ws/install/setup.bash; exec \"$0\" \"$@\""]
CMD ["bash"]
