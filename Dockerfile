FROM ros:jazzy-ros-base AS base

RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive \
    apt-get install -y \
    ros-jazzy-foxglove-bridge \
    cmake \
    build-essential \
    wget \
    unzip \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

ARG LIVOX_SDK2_TAG=v1.3.1
RUN wget -q "https://github.com/Livox-SDK/Livox-SDK2/archive/refs/tags/${LIVOX_SDK2_TAG}.zip" -O /tmp/livox-sdk2.zip \
 && unzip -q /tmp/livox-sdk2.zip -d /tmp \
 && SHORT_TAG=$(echo "${LIVOX_SDK2_TAG}" | sed 's/^v//') \
 && cd "/tmp/Livox-SDK2-${SHORT_TAG}" \
 && mkdir -p build && cd build \
 && cmake .. -DCMAKE_CXX_FLAGS="-include cstdint" \
 && make -j$(nproc) \
 && make install \
 && cd / \
 && rm -rf /tmp/livox-sdk2.zip "/tmp/Livox-SDK2-${SHORT_TAG}"

# zed_f9r (u-blox ZED-F9R) 用。sparkfun-ublox-gps は rosdep に無いので pip で入れる。
# pyserial / spidev はライブラリが無条件に import するが依存宣言されていないため明示する。
RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y python3-pip \
 && pip3 install --break-system-packages --no-cache-dir \
    sparkfun-ublox-gps \
    pyserial \
    spidev \
 && apt-get clean && rm -rf /var/lib/apt/lists/*

FROM base AS build
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
  python3-colcon-common-extensions \
  ros-jazzy-ament-lint \
  && apt-get clean && rm -rf /var/lib/apt/lists/*

COPY . /ros2_ws
WORKDIR /ros2_ws
SHELL [ "/bin/bash", "-c" ]

RUN source /opt/ros/jazzy/setup.bash && colcon build

FROM base AS deploy
COPY --from=build /opt/ros/jazzy /opt/ros/jazzy

FROM base AS devenv
RUN apt-get update && apt-get install -y \
  python3-colcon-common-extensions \
  git \
  ros-jazzy-foxglove-bridge \
  python3-pip \
  ros-jazzy-desktop \
  mosquitto-clients \
  mosquitto \
  ros-jazzy-realsense2-* \
  ros-jazzy-usb-cam \
  && apt-get clean && rm -rf /var/lib/apt/lists/*

# COPY .devcontainer/requirements.txt /requirements.txt
# RUN pip3 install --break-system-packages --ignore-installed -r /requirements.txt

# ARG USERNAME=vscode
# ARG USER_UID=1000
# ARG USER_GID=1000

# RUN userdel -r ubuntu 2>/dev/null || true \
#     && groupadd --gid $USER_GID $USERNAME \
#     && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
#     && apt-get update \
#     && apt-get install -y sudo \
#     && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
#     && chmod 0440 /etc/sudoers.d/$USERNAME \
#     && usermod -aG video,dialout,plugdev $USERNAME

# USER $USERNAME
# RUN echo "source /opt/ros/jazzy/setup.bash" >> ~/.bashrc
# RUN curl -fsSL https://claude.ai/install.sh | bash
# RUN echo "export PATH=/home/vscode/.local/bin:\$PATH" >> ~/.bashrc
