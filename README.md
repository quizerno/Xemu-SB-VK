Fork of faha223/xemu 

While adding features for Steel Battalion, the ultimate goal of this is to create two seamless versions of xemu fork of usb_passthrough, one that uses vulkan and one that doesn't use vulkan.

This project is part of my Steel Battalion Newcomer guide which focuses on the controller.

**Features**
* Option to turn On/Off hotkeys
* Removes double click toggling fullscreen
* New Hotkeys for Displaying/Hiding menu and cursor
* Fully rebindable keyboard keys for Steel Battalion within the emulator
* Minor fixes such as Steel Battalion Controller Graphic Display Improvements

**To Do**
* Add all features to the non-vulkan branch version
* Fix Bug where Steel Battalion Controller Graphic Display does not show rebinds until restart
* Add Full Mouse Rebinding, Including Mousewheel.
* Add Hotkey Rebinding
* See if Keyboard+Gamepad combination play is possible
* Display cursor when settings menu is open (add from other build)
* Implimentation of joystick additions that FluffyStuff added in his build
* Including instructions for people who want to build on their own 


**Bugs**
List here




**Build Snippits**
git clone -b usb_passthrough_vk https://github.com/faha223/xemu.git [optional directory name] --recurse-submodules
git clone -b usb_passthrough_vk https://github.com/avibodek/Xemu-SB-VK.git [optional directory name] --recurse-submodules


git clone -b usb_passthrough https://github.com/faha223/xemu.git  [optional directory name] --recurse-submodules
git clone -b usb_passthrough https://github.com/avibodek/Xemu-SB-VK.git [optional directory name] --recurse-submodules


git clone -b usb_passthrough_vk https://github.com/faha223/xemu.git [optional directory name] --recurse-submodules
Building vk




docker run --rm -v $PWD/Xemu-SB-VK:/xemu -w /xemu \
    -e CCACHE_DIR=/xemu/ccache --platform linux/amd64 \
    ghcr.io/xemu-project/xemu-win64-toolchain:latest \
    /bin/bash -c "apt-get update && apt-get install -qy curl && ./build.sh -p win64-cross"

Building non-Vk


docker run --rm -v $PWD/xemu:/xemu -w /xemu \
    -e CCACHE_DIR=/xemu/ccache --platform linux/amd64 \
    ghcr.io/xemu-project/xemu-win64-toolchain:sha-8152913 \
    /bin/bash -c "cd /opt/mxe && \
    make MXE_TARGETS=x86_64-w64-mingw32.static MXE_PLUGIN_DIRS=plugins/gcc13 libusb1 && \
    cd /xemu && \
    ./build.sh -p win64-cross --enable-libusb"
