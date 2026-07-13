# Intro
Fork of faha223/xemu 

While adding features for Steel Battalion, the ultimate goal of this is to create two seamless versions of xemu fork of usb_passthrough, one that uses vulkan and one that doesn't use vulkan.

This project is part of my Steel Battalion Newcomer guide which focuses on the controller. I am working with SpecialFred.

**Limitations**
* For OG Steel Battalion only the first four level are effectively playable (on Vulkan)
* Struggling with LoC optimization

**Features**
* Option to turn On/Off hotkeys
* Option to turn On/Off click toggling fullscreen
* New Hotkeys for Displaying/Hiding menu and cursor
* Fully rebindable keyboard keys for Steel Battalion within the emulator (requires restart for now)
* Minor fixes such as Steel Battalion Controller Graphic Display Improvements

**To Do**
* Add all features to the non-vulkan branch version
* Make non-vulkan and vulkan have the same hotkeys
* Fix Bug where Keyboard Steel Battalion Controller does not rebind until restart
* Add Full Mouse Rebinding, Including Mousewheel.
* Add Hotkey Rebinding
* See if Keyboard+Gamepad combination play is possible
* Add predefined keyboard maps for both Full Keyboards and 80% Keyboards.
* Implimentation of joystick additions that FluffyStuff added in his build
* Including instructions for people who want to build on their own.
* Proper documentation for interactions with libusb: https://github.com/faha223/XboxPeripheralLibusbTools/


**Bugs**
List here

# How to Build
Build in linux or WSL, be sure to install podman or docker. You must be in the directory with the build folder to build.
You may substitude Xemu-SB-VK or Xemu-SB-NVK with any directory name you want. The default directory will be "xemu" otherwise.

Other information on building Xemu can be found here: https://xemu.app/docs/dev/building-from-source/

## Building Vulkan (for Windows)
Clone with:

`git clone -b usb_passthrough_vk https://github.com/faha223/xemu.git Xemu-SB-VK --recurse-submodules`

Then build with:
```
docker run --rm -v $PWD/Xemu-SB-VK:/xemu -w /xemu \
    -e CCACHE_DIR=/xemu/ccache --platform linux/amd64 \
    ghcr.io/xemu-project/xemu-win64-toolchain:latest \
    /bin/bash -c "apt-get update && apt-get install -qy curl && ./build.sh -p win64-cross"
```

Run with:

`    ./dist/xemu`

## Building Non-Vulkan (for windows)

Clone with:

`git clone -b usb_passthrough https://github.com/avibodek/Xemu-SB-VK.git Xemu-SB-NVK --recurse-submodules`

Then build with:

```
docker run --rm -v $PWD/Xemu-SB-NVK:/xemu -w /xemu \
    -e CCACHE_DIR=/xemu/ccache --platform linux/amd64 \
    ghcr.io/xemu-project/xemu-win64-toolchain:sha-8152913 \
    /bin/bash -c "cd /opt/mxe && \
    make MXE_TARGETS=x86_64-w64-mingw32.static MXE_PLUGIN_DIRS=plugins/gcc13 libusb1 && \
    cd /xemu && \
    ./build.sh -p win64-cross --enable-libusb"
```
    
Run with:

`./dist/xemu`

## Notes on Adding Settings to the Menu

**/config_spec.yml** - Global variables including default control scheme

**//ui/xui/main-menu.cc** -- Adding options to the menu

**//ui/xui/menubar.cc** -- Some hotkey listeners

**//ui/xui/main.cc** -- Some hotkey listeners

**//ui/xui/actions.cc** -- Some notifications

## Other helpful links

https://github.com/Ryzee119/ogx360_t4/


