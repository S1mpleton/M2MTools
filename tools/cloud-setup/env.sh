# Source this file in each new shell; processes and exports do not survive snapshots.
export PATH="/workspace/.cloud-setup/sysroot/usr/bin:$PATH"
export LD_LIBRARY_PATH="/workspace/.cloud-setup/sysroot/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="/workspace/.cloud-setup/sysroot/usr/lib/x86_64-linux-gnu/qt6/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
export XDG_CACHE_HOME="/workspace/.cloud-setup/cache"
