#!/bin/bash
set -euo pipefail
cd /workspace/M2MTools
test "$(dpkg --print-architecture)" = amd64
test -r /workspace/.cloud-setup/debian.sources
test -r /workspace/.cloud-setup/apt.conf
mkdir -p /workspace/.cloud-setup/apt/lists/partial \
  /workspace/.cloud-setup/apt/cache/archives/partial \
  /workspace/.cloud-setup/apt/empty.conf.d \
  /workspace/.cloud-setup/sysroot /workspace/.cloud-setup/cache \
  /workspace/.cloud-setup/run
export APT_CONFIG=/workspace/.cloud-setup/apt.conf
apt-get update
apt-get -y --download-only --no-install-recommends install \
  cmake ninja-build qt6-base-dev qt6-tools-dev \
  qt6-tools-dev-tools qt6-l10n-tools libqt6sql6-sqlite libssl-dev
# The CMake targets and development symlinks need these runtime files inside
# the relocated prefix even when the base machine has already installed them.
cd /workspace/.cloud-setup/apt/cache/archives
apt-get download libqt6core6t64 libqt6dbus6 libqt6gui6 libqt6network6 \
  libqt6opengl6 libqt6openglwidgets6 libqt6printsupport6 libqt6sql6 \
  libqt6widgets6 libqt6xml6 libgl1 libglx0 libopengl0 libglvnd0 \
  libssl3t64 libqt6test6
for package_file in /workspace/.cloud-setup/apt/cache/archives/*.deb; do
  dpkg-deb -x "$package_file" /workspace/.cloud-setup/sysroot
done
source /workspace/.cloud-setup/env.sh
cd /workspace/M2MTools
cmake -S /workspace/M2MTools -B /workspace/.cloud-setup/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=/workspace/.cloud-setup/sysroot/usr \
  -DOPENSSL_ROOT_DIR=/workspace/.cloud-setup/sysroot/usr \
  -DOPENSSL_CRYPTO_LIBRARY=/workspace/.cloud-setup/sysroot/usr/lib/x86_64-linux-gnu/libcrypto.so \
  -DOPENSSL_INCLUDE_DIR=/workspace/.cloud-setup/sysroot/usr/include \
  -DBUILD_TESTING=ON
cmake --build /workspace/.cloud-setup/build --parallel 2
ctest --test-dir /workspace/.cloud-setup/build --output-on-failure
