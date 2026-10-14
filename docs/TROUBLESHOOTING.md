# Troubleshooting Guide

This guide covers common issues, diagnosis techniques, and resolutions when building, testing, or deploying the Embedded UI Designer Platform.

## 1. Build Issues

### macOS: `xcrun: error: invalid active developer path`
- **Cause**: Xcode Command Line Tools are missing or need updating following a macOS update.
- **Solution**:
  ```bash
  xcode-select --install
  sudo xcode-select --reset
  ```

### Linux: `fatal error: json/json.h: No such file or directory`
- **Cause**: The `libjsoncpp-dev` package is not installed.
- **Solution**:
  ```bash
  sudo apt-get update && sudo apt-get install -y libjsoncpp-dev
  ```

### Windows: `ninja: error: loading 'build.ninja': The system cannot find the file specified`
- **Cause**: CMake configuration step failed or was skipped before build.
- **Solution**:
  ```powershell
  cmake -G "Ninja" -DENABLE_TESTS=ON ..
  ninja
  ```

## 2. Hardware & OpenOCD Issues

### ST-Link: `Error: open failed: unable to open ST-LINK device`
- **Cause**: ST-Link probe is unplugged, in use by another debugger, or Linux udev rules are missing.
- **Solution**:
  - Unplug and reconnect the ST-Link probe.
  - Install standard OpenOCD udev rules:
    ```bash
    sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
    sudo udevadm control --reload-rules && sudo udevadm trigger
    ```

### Serial Monitor: `Permission denied: '/dev/ttyUSB0'`
- **Cause**: User account is not in the `dialout` or `uucp` group.
- **Solution**:
  ```bash
  sudo usermod -a -G dialout $USER
  # Log out and log back in for changes to take effect
  ```

## 3. Test Failures & Debugging

### Test fails with `symbol not found` or stale cache
- **Solution**:
  ```bash
  rm -rf build
  mkdir build && cd build
  cmake -DENABLE_TESTS=ON ..
  make -j$(nproc)
  ctest --verbose --output-on-failure
  ```
