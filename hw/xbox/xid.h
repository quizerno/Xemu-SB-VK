#ifndef __XID_H__
#define __XID_H__

/*
 * QEMU USB XID Devices
 *
 * Copyright (c) 2013 espes
 * Copyright (c) 2017 Jannik Vogel
 * Copyright (c) 2018-2021 Matt Borgerson
 * Copyright (c) 2023 Fred Hallock
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/qdev-properties.h"
#include "hw/usb.h"
#include "hw/usb/desc.h"
#include "migration/vmstate.h"
#include "system/system.h"
#include "ui/console.h"
#include "ui/xemu-input.h"

// #define DEBUG_XID
#ifdef DEBUG_XID
#define DPRINTF printf
#else
#define DPRINTF(...)
#endif

#define USB_CLASS_XID 0x58
#define USB_DT_XID 0x42

#define HID_GET_REPORT 0x01
#define HID_SET_REPORT 0x09
#define XID_GET_CAPABILITIES 0x01

#define XID_DEVICETYPE_GAMEPAD 0x01
#define XID_DEVICETYPE_STEEL_BATTALION 0x80

#define XID_DEVICESUBTYPE_GAMEPAD 0x01
#define XID_DEVICESUBTYPE_GAMEPAD_S 0x02

#define TYPE_USB_XID_GAMEPAD "usb-xbox-gamepad"
#define TYPE_USB_XID_GAMEPAD_S "usb-xbox-gamepad-s"
#define TYPE_USB_XID_STEEL_BATTALION "usb-steel-battalion"

#define GAMEPAD_A 0
#define GAMEPAD_B 1
#define GAMEPAD_X 2
#define GAMEPAD_Y 3
#define GAMEPAD_BLACK 4
#define GAMEPAD_WHITE 5
#define GAMEPAD_LEFT_TRIGGER 6
#define GAMEPAD_RIGHT_TRIGGER 7

#define GAMEPAD_DPAD_UP 8
#define GAMEPAD_DPAD_DOWN 9
#define GAMEPAD_DPAD_LEFT 10
#define GAMEPAD_DPAD_RIGHT 11
#define GAMEPAD_START 12
#define GAMEPAD_BACK 13
#define GAMEPAD_LEFT_THUMB 14
#define GAMEPAD_RIGHT_THUMB 15

#define BUTTON_MASK(button) (1 << ((button) - GAMEPAD_DPAD_UP))

enum {
    STR_MANUFACTURER = 1,
    STR_PRODUCT,
    STR_SERIALNUMBER,
};

extern const USBDescStrings desc_strings;

typedef struct XIDDesc {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdXid;
    uint8_t bType;
    uint8_t bSubType;
    uint8_t bMaxInputReportSize;
    uint8_t bMaxOutputReportSize;
    uint16_t wAlternateProductIds[4];
} QEMU_PACKED XIDDesc;

typedef struct XIDGamepadReport {
    uint8_t bReportId;
    uint8_t bLength;
    uint16_t wButtons;
    uint8_t bAnalogButtons[8];
    int16_t sThumbLX;
    int16_t sThumbLY;
    int16_t sThumbRX;
    int16_t sThumbRY;
} QEMU_PACKED XIDGamepadReport;

typedef struct XIDGamepadOutputReport {
    uint8_t report_id; // FIXME: is this correct?
    uint8_t length;
    uint16_t left_actuator_strength;
    uint16_t right_actuator_strength;
} QEMU_PACKED XIDGamepadOutputReport;

typedef struct USBXIDGamepadState {
    USBDevice dev;
    USBEndpoint *intr;
    const XIDDesc *xid_desc;
    XIDGamepadReport in_state;
    XIDGamepadReport in_state_capabilities;
    XIDGamepadOutputReport out_state;
    XIDGamepadOutputReport out_state_capabilities;
    uint8_t device_index;
} USBXIDGamepadState;

typedef struct XIDSteelBattalionReport {
    uint8_t bReportId;
    uint8_t bLength;
    uint32_t dwButtons;
    uint8_t bMoreButtons;
    uint16_t wPadding;
    uint8_t bAimingX;
    uint8_t bPadding;
    uint8_t bAimingY;
    int16_t sRotationLever; // only high byte is used
    int16_t sSightChangeX; // only high byte is used
    int16_t sSightChangeY; // only high byte is used
    uint16_t wLeftPedal; // only high byte is used
    uint16_t wMiddlePedal; // only high byte is used
    uint16_t wRightPedal; // only high byte is used
    uint8_t ucTunerDial; // low nibble, The 9 o'clock postion is 0, and the 6
                         // o'clock position is 12
    uint8_t ucGearLever; // gear lever 1~5 for gear 1~5, 7~13 for gear R,N,1~5,
                         // 15 for gear R
} QEMU_PACKED XIDSteelBattalionReport;

// Based on:
// https://github.com/Ryzee119/ogx360/blob/master/Firmware/src/usbd/usbd_xid.h:195
typedef struct XIDSteelBattalionOutputReport {
    uint8_t report_id;
    uint8_t length;
    uint8_t EmergencyEject : 4;
    uint8_t CockpitHatch : 4;
    uint8_t Ignition : 4;
    uint8_t Start : 4;
    uint8_t OpenClose : 4;
    uint8_t MapZoomInOut : 4;
    uint8_t ModeSelect : 4;
    uint8_t SubMonitorModeSelect : 4;
    uint8_t MainMonitorZoomIn : 4;
    uint8_t MainMonitorZoomOut : 4;
    uint8_t ForecastShootingSystem : 4;
    uint8_t Manipulator : 4;
    uint8_t LineColorChange : 4;
    uint8_t Washing : 4;
    uint8_t Extinguisher : 4;
    uint8_t Chaff : 4;
    uint8_t TankDetach : 4;
    uint8_t Override : 4;
    uint8_t NightScope : 4;
    uint8_t F1 : 4;
    uint8_t F2 : 4;
    uint8_t F3 : 4;
    uint8_t MainWeaponControl : 4;
    uint8_t SubWeaponControl : 4;
    uint8_t MagazineChange : 4;
    uint8_t Comm1 : 4;
    uint8_t Comm2 : 4;
    uint8_t Comm3 : 4;
    uint8_t Comm4 : 4;
    uint8_t Comm5 : 4;
    uint8_t : 4;
    uint8_t GearR : 4;
    uint8_t GearN : 4;
    uint8_t Gear1 : 4;
    uint8_t Gear2 : 4;
    uint8_t Gear3 : 4;
    uint8_t Gear4 : 4;
    uint8_t Gear5 : 4;
    uint8_t not_used;
} QEMU_PACKED XIDSteelBattalionOutputReport;

typedef struct USBXIDSteelBattalionState {
    USBDevice dev;
    USBEndpoint *intr;
    const XIDDesc *xid_desc;
    XIDSteelBattalionReport in_state;
    XIDSteelBattalionReport in_state_capabilities;
    XIDSteelBattalionOutputReport out_state;
    XIDSteelBattalionOutputReport out_state_capabilities;
    uint8_t device_index;
} USBXIDSteelBattalionState;

void update_input(USBXIDGamepadState *s);
void update_output(USBXIDGamepadState *s);
void usb_xid_handle_reset(USBDevice *dev);
void usb_xid_handle_control(USBDevice *dev, USBPacket *p, int request,
                            int value, int index, int length, uint8_t *data);
void usb_xbox_gamepad_unrealize(USBDevice *dev);

#if 0
void usb_xid_handle_destroy(USBDevice *dev);
#endif

#endif