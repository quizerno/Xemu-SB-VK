# Intro
Fork of faha223/xemu 

While adding features for Steel Battalion, the ultimate goal of this is to create two seamless versions of xemu fork of usb_passthrough, one that uses vulkan (VK) and one that doesn't use vulkan (NVK).

This project is part of my Steel Battalion Newcomer guide which focuses on the controller. I am working with SpecialFred.

**Limitations**
* For OG Steel Battalion performance is not consistent, some people have run into the only the first 4 levels being consistently playable, while others have been able to play most of the game.
* Struggling with LoC optimization
* Directly binding to joysticks is currently not possible, by using a rebinder such as JoystickGremlin you can bind the keyboard configuration to any joystick of your choice.

**Features**
* Both: Option to turn On/Off hotkeys
* Both: Option to turn On/Off click toggling fullscreen
* Both: New Hotkeys for Displaying/Hiding menu and cursor
* Both: Minor fixes such as Steel Battalion Controller Graphic Display Improvements
* VK: Fully rebindable keyboard keys for Steel Battalion within the emulator (requires restart for now)

**To Do**
* Add all input-rebinding to the non-vulkan branch version
* Make non-vulkan and vulkan have the same hotkeys
* Impliment options for Gear Shifter and Tuner Dial to be changed to invididual selectors instead of incremental buttons.
* Add Full Mouse Rebinding, Including Mousewheel.
* Add Hotkey Rebinding
* Add predefined keyboard maps for both Full Keyboards and 80% Keyboards.
* Including instructions for people who want to build on their own.

**Look Into**
* Light event feedback for RGB keyboards and other devices
* Implimentation of joystick additions that FluffyStuff added in his build
* See if Keyboard+Gamepad combination play is possible without needing a rebinder
* Proper documentation for interactions with libusb: https://github.com/faha223/XboxPeripheralLibusbTools/


**Bugs**
* Keyboard Steel Battalion Controller does not rebind until restart
* SB: Light events not fully recognized in Mission 00

**Differences between VK and non-VK**
* VK uses SDL3, Non-VK uses SDL2
* VK settings menu does not graphically show SB inputs when doing usb_passthrough, NVK does


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

## Building Non-Vulkan (for Windows)

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

* https://github.com/Ryzee119/ogx360_t4/

* https://github.com/SantiagoSaldana/SBC

* https://github.com/faha223/XboxPeripheralLibusbTools/

* https://xemu.app/docs/dev/building-from-source/

* https://github.com/dmadison/ArduinoXInput_Teensy
