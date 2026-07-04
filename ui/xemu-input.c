/*
 * xemu Input Management
 *
 * Copyright (C) 2020-2021 Matt Borgerson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "qemu/osdep.h"
#include "hw/usb.h"
#include "hw/qdev-core.h"
#include "hw/qdev-properties.h"
#include "qapi/error.h"
#include "monitor/qdev.h"
#include "qobject/qdict.h"
#include "qemu/option.h"
#include "qemu/timer.h"
#include "qemu/config-file.h"

#include "xemu-input.h"
#include "xemu-notifications.h"
#include "xemu-settings.h"
#include <stdio.h>
#include <stdlib.h>

#include "system/blockdev.h"

extern SDL_Window *m_window;
extern int viewport_coords[4];

<<<<<<< HEAD
/* //quizerno
bool sight_on;
bool aim_on;
//bool aim_off;
int32_t last_aim_x;
int32_t last_aim_y; */

=======
>>>>>>> c18c45e74d0605912917c251118a9d17582cabc1
#define SBC_PEDAL_MAX 0xFFC0

// #define DEBUG_INPUT

#ifdef DEBUG_INPUT
#define DPRINTF(fmt, ...) \
    do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

#define XEMU_INPUT_MIN_INPUT_UPDATE_INTERVAL_US  2500
#define XEMU_INPUT_MIN_RUMBLE_UPDATE_INTERVAL_US 2500

#if 0
static void xemu_input_print_controller_state(ControllerState *state)
{
    DPRINTF("     A = %d,      B = %d,     X = %d,     Y = %d\n"
           "  Left = %d,     Up = %d, Right = %d,  Down = %d\n"
           "  Back = %d,  Start = %d, White = %d, Black = %d\n"
           "Lstick = %d, Rstick = %d, Guide = %d\n"
           "\n"
           "LTrig   = %.3f, RTrig   = %.3f\n"
           "LStickX = %.3f, RStickX = %.3f\n"
           "LStickY = %.3f, RStickY = %.3f\n\n",
        !!(state->gp.buttons & CONTROLLER_BUTTON_A),
        !!(state->gp.buttons & CONTROLLER_BUTTON_B),
        !!(state->gp.buttons & CONTROLLER_BUTTON_X),
        !!(state->gp.buttons & CONTROLLER_BUTTON_Y),
        !!(state->gp.buttons & CONTROLLER_BUTTON_DPAD_LEFT),
        !!(state->gp.buttons & CONTROLLER_BUTTON_DPAD_UP),
        !!(state->gp.buttons & CONTROLLER_BUTTON_DPAD_RIGHT),
        !!(state->gp.buttons & CONTROLLER_BUTTON_DPAD_DOWN),
        !!(state->gp.buttons & CONTROLLER_BUTTON_BACK),
        !!(state->gp.buttons & CONTROLLER_BUTTON_START),
        !!(state->gp.buttons & CONTROLLER_BUTTON_WHITE),
        !!(state->gp.buttons & CONTROLLER_BUTTON_BLACK),
        !!(state->gp.buttons & CONTROLLER_BUTTON_LSTICK),
        !!(state->gp.buttons & CONTROLLER_BUTTON_RSTICK),
        !!(state->gp.buttons & CONTROLLER_BUTTON_GUIDE),
        state->gp.axis[CONTROLLER_AXIS_LTRIG],
        state->gp.axis[CONTROLLER_AXIS_RTRIG],
        state->gp.axis[CONTROLLER_AXIS_LSTICK_X],
        state->gp.axis[CONTROLLER_AXIS_RSTICK_X],
        state->gp.axis[CONTROLLER_AXIS_LSTICK_Y],
        state->gp.axis[CONTROLLER_AXIS_RSTICK_Y]
        );
}
#endif

ControllerStateList available_controllers =
    QTAILQ_HEAD_INITIALIZER(available_controllers);

ControllerState *bound_controllers[4] = { NULL, NULL, NULL, NULL };
LibusbDevice *bound_libusb_devices[4] = { NULL, NULL, NULL, NULL };
const char *all_drivers[4] = { DRIVER_DUKE, DRIVER_S, DRIVER_STEEL_BATTALION, DRIVER_USB_PASSTHROUGH };
const char *bound_drivers[4] = { DRIVER_DUKE, DRIVER_DUKE, DRIVER_DUKE, DRIVER_DUKE };
int test_mode;

static const char **port_index_to_settings_key_map[] = {
    &g_config.input.bindings.port1,
    &g_config.input.bindings.port2,
    &g_config.input.bindings.port3,
    &g_config.input.bindings.port4,
};

static const char **port_index_to_driver_settings_key_map[] = {
    &g_config.input.bindings.port1_driver,
    &g_config.input.bindings.port2_driver,
    &g_config.input.bindings.port3_driver, 
    &g_config.input.bindings.port4_driver
};

static int *peripheral_types_settings_map[4][2] = {
    { &g_config.input.peripherals.port1.peripheral_type_0,
      &g_config.input.peripherals.port1.peripheral_type_1 },
    { &g_config.input.peripherals.port2.peripheral_type_0,
      &g_config.input.peripherals.port2.peripheral_type_1 },
    { &g_config.input.peripherals.port3.peripheral_type_0,
      &g_config.input.peripherals.port3.peripheral_type_1 },
    { &g_config.input.peripherals.port4.peripheral_type_0,
      &g_config.input.peripherals.port4.peripheral_type_1 }
};

static const char **peripheral_params_settings_map[4][2] = {
    { &g_config.input.peripherals.port1.peripheral_param_0,
      &g_config.input.peripherals.port1.peripheral_param_1 },
    { &g_config.input.peripherals.port2.peripheral_param_0,
      &g_config.input.peripherals.port2.peripheral_param_1 },
    { &g_config.input.peripherals.port3.peripheral_param_0,
      &g_config.input.peripherals.port3.peripheral_param_1 },
    { &g_config.input.peripherals.port4.peripheral_param_0,
      &g_config.input.peripherals.port4.peripheral_param_1 }
};

static int sdl_sbc_kbd_scancode_map[52];

<<<<<<< HEAD

int *g_keyboard_sbc_scancode_map[49] = {
		  &g_config.input.keyboard_sbc_scancode_map.eject,
          &g_config.input.keyboard_sbc_scancode_map.cockpit_hatch,
          &g_config.input.keyboard_sbc_scancode_map.ignition,
          &g_config.input.keyboard_sbc_scancode_map.start,
          &g_config.input.keyboard_sbc_scancode_map.open_close, 
          &g_config.input.keyboard_sbc_scancode_map.map_zoom_in_out, 
          &g_config.input.keyboard_sbc_scancode_map.mode_select,
          &g_config.input.keyboard_sbc_scancode_map.sub_monitor_mode_select,
          &g_config.input.keyboard_sbc_scancode_map.zoom_in,
          &g_config.input.keyboard_sbc_scancode_map.zoom_out, 
          &g_config.input.keyboard_sbc_scancode_map.fss,
          &g_config.input.keyboard_sbc_scancode_map.manipulator, 
          &g_config.input.keyboard_sbc_scancode_map.line_color_change,
          &g_config.input.keyboard_sbc_scancode_map.washing,
          &g_config.input.keyboard_sbc_scancode_map.extinguisher, 
          &g_config.input.keyboard_sbc_scancode_map.chaff,
          &g_config.input.keyboard_sbc_scancode_map.tank_detach, 
          &g_config.input.keyboard_sbc_scancode_map.override,
          &g_config.input.keyboard_sbc_scancode_map.night_scope,
          &g_config.input.keyboard_sbc_scancode_map.func1, 
          &g_config.input.keyboard_sbc_scancode_map.func2,
          &g_config.input.keyboard_sbc_scancode_map.func3,
          &g_config.input.keyboard_sbc_scancode_map.main_weapon_control,
          &g_config.input.keyboard_sbc_scancode_map.sub_weapon_control,
          &g_config.input.keyboard_sbc_scancode_map.magazine_change,
          &g_config.input.keyboard_sbc_scancode_map.com1, 
          &g_config.input.keyboard_sbc_scancode_map.com2,            
          &g_config.input.keyboard_sbc_scancode_map.com3,
          &g_config.input.keyboard_sbc_scancode_map.com4,
          &g_config.input.keyboard_sbc_scancode_map.com5,
          &g_config.input.keyboard_sbc_scancode_map.sight_change,
          &g_config.input.keyboard_sbc_scancode_map.filt_control_system,
          &g_config.input.keyboard_sbc_scancode_map.oxygen_supply_system,
          &g_config.input.keyboard_sbc_scancode_map.fuel_flow_rate, 
          &g_config.input.keyboard_sbc_scancode_map.buffer_material,
          &g_config.input.keyboard_sbc_scancode_map.vt_location_measurement,
          &g_config.input.keyboard_sbc_scancode_map.gear_up,
          &g_config.input.keyboard_sbc_scancode_map.gear_down,
          &g_config.input.keyboard_sbc_scancode_map.tuner_left,
          &g_config.input.keyboard_sbc_scancode_map.tuner_right,
 		  &g_config.input.keyboard_sbc_scancode_map.sight_change_up,
          &g_config.input.keyboard_sbc_scancode_map.sight_change_down,
          &g_config.input.keyboard_sbc_scancode_map.sight_change_left,
		  &g_config.input.keyboard_sbc_scancode_map.sight_change_right,
		  &g_config.input.keyboard_sbc_scancode_map.rotation_left,
          &g_config.input.keyboard_sbc_scancode_map.rotation_right,
          &g_config.input.keyboard_sbc_scancode_map.left_pedal,
          &g_config.input.keyboard_sbc_scancode_map.right_pedal,		  
		  &g_config.input.keyboard_sbc_scancode_map.middle_pedal,            

/*           &state->sbc_controller_map->keyboard_sbc_scancode_map.sight_change_up,
          &state->sbc_controller_map->keyboard_sbc_scancode_map.sight_change_down,
          &state->sbc_controller_map->keyboard_sbc_scancode_map.sight_change_left,
          &state->sbc_controller_map->keyboard_sbc_scancode_map.sight_change_right,
          &state->sbc_controller_map->keyboard_sbc_scancode_map.rotation_left,
          &state->sbc_controller_map->keyboard_sbc_scancode_map.rotation_right,
          &state->sbc_controller_map->keyboard_sbc_scancode_map.left_pedal,
          &state->sbc_controller_map->keyboard_sbc_scancode_map.right_pedal,            
          &state->sbc_controller_map->keyboard_sbc_scancode_map.middle_pedal,	 */	  
		  
		  
		
};

=======
>>>>>>> c18c45e74d0605912917c251118a9d17582cabc1
int *g_keyboard_scancode_map[25] = {
    &g_config.input.keyboard_controller_scancode_map.a,
    &g_config.input.keyboard_controller_scancode_map.b,
    &g_config.input.keyboard_controller_scancode_map.x,
    &g_config.input.keyboard_controller_scancode_map.y,
    &g_config.input.keyboard_controller_scancode_map.back,
    &g_config.input.keyboard_controller_scancode_map.guide,
    &g_config.input.keyboard_controller_scancode_map.start,
    &g_config.input.keyboard_controller_scancode_map.lstick_btn,
    &g_config.input.keyboard_controller_scancode_map.rstick_btn,
    &g_config.input.keyboard_controller_scancode_map.white,
    &g_config.input.keyboard_controller_scancode_map.black,
    &g_config.input.keyboard_controller_scancode_map.dpad_up,
    &g_config.input.keyboard_controller_scancode_map.dpad_down,
    &g_config.input.keyboard_controller_scancode_map.dpad_left,
    &g_config.input.keyboard_controller_scancode_map.dpad_right,
    &g_config.input.keyboard_controller_scancode_map.lstick_up,
    &g_config.input.keyboard_controller_scancode_map.lstick_left,
    &g_config.input.keyboard_controller_scancode_map.lstick_right,
    &g_config.input.keyboard_controller_scancode_map.lstick_down,
    &g_config.input.keyboard_controller_scancode_map.ltrigger,
    &g_config.input.keyboard_controller_scancode_map.rstick_up,
    &g_config.input.keyboard_controller_scancode_map.rstick_left,
    &g_config.input.keyboard_controller_scancode_map.rstick_right,
    &g_config.input.keyboard_controller_scancode_map.rstick_down,
    &g_config.input.keyboard_controller_scancode_map.rtrigger,
};


static void check_and_reset_in_range(int *btn, int min, int max,
                                     const char *message)
{
    if (*btn < min || *btn >= max) {
        fprintf(stderr, "%s\n", message);
        *btn = min;
    }
}

static void xemu_input_bindings_set_in_range(ControllerState *con)
{
#define CHECK_RESET_BUTTON(btn)                                            \
    check_and_reset_in_range(&con->controller_map->controller_mapping.btn, \
                             SDL_GAMEPAD_BUTTON_INVALID,                   \
                             SDL_GAMEPAD_BUTTON_COUNT,                     \
                             "Invalid entry for button " #btn ", resetting")

    CHECK_RESET_BUTTON(a);
    CHECK_RESET_BUTTON(b);
    CHECK_RESET_BUTTON(x);
    CHECK_RESET_BUTTON(y);
    CHECK_RESET_BUTTON(dpad_left);
    CHECK_RESET_BUTTON(dpad_up);
    CHECK_RESET_BUTTON(dpad_right);
    CHECK_RESET_BUTTON(dpad_down);
    CHECK_RESET_BUTTON(back);
    CHECK_RESET_BUTTON(start);
    CHECK_RESET_BUTTON(lshoulder);
    CHECK_RESET_BUTTON(rshoulder);
    CHECK_RESET_BUTTON(lstick_btn);
    CHECK_RESET_BUTTON(rstick_btn);
    CHECK_RESET_BUTTON(guide);

#undef CHECK_RESET_BUTTON

#define CHECK_RESET_AXIS(axis)                                              \
    check_and_reset_in_range(&con->controller_map->controller_mapping.axis, \
                             SDL_GAMEPAD_AXIS_INVALID,                      \
                             SDL_GAMEPAD_AXIS_COUNT,                        \
                             "Invalid entry for button " #axis ", resetting")

    CHECK_RESET_AXIS(axis_trigger_left);
    CHECK_RESET_AXIS(axis_trigger_right);
    CHECK_RESET_AXIS(axis_left_x);
    CHECK_RESET_AXIS(axis_left_y);
    CHECK_RESET_AXIS(axis_right_x);
    CHECK_RESET_AXIS(axis_right_y);

#undef CHECK_RESET_AXIS
}

static void xemu_input_bindings_reload_map(ControllerState *con)
{
    assert(con->type == INPUT_DEVICE_SDL_GAMEPAD);

    char guid[35] = { 0 };
    SDL_GUIDToString(con->sdl_joystick_guid, guid, sizeof(guid));
    if (!xemu_settings_load_gamepad_mapping(guid, &con->controller_map)) {
        return;
    }

    // If this controller did not exist in the mapping array, the config will
    // have been reallocated. Any gamepad mapping pointers for other controllers
    // are now invalid, and need to be reloaded.
    ControllerState *iter, *next;
    bool is_new_mapping;
    QTAILQ_FOREACH_SAFE (iter, &available_controllers, entry, next) {
        if (iter == con || iter->type != INPUT_DEVICE_SDL_GAMEPAD) {
            continue;
        }

        memset(guid, 0, sizeof(guid));
        SDL_GUIDToString(iter->sdl_joystick_guid, guid, sizeof(guid));

        is_new_mapping =
            xemu_settings_load_gamepad_mapping(guid, &iter->controller_map);
        assert(!is_new_mapping &&
               "Existing controller GUIDs should exist in the config");

        xemu_input_bindings_set_in_range(iter);
    }
}

static const char *get_bound_driver(int port)
{
    assert(port >= 0 && port <= 3);
    const char *driver = *port_index_to_driver_settings_key_map[port];

    // If the driver in the config is NULL, empty, or unrecognized 
    // then default to DRIVER_DUKE
    if (driver == NULL)
        return DRIVER_DUKE; 
    if (strlen(driver) == 0)
        return DRIVER_DUKE;
    if (strcmp(driver, DRIVER_DUKE) == 0)
        return DRIVER_DUKE;
    if (strcmp(driver, DRIVER_S) == 0)
        return DRIVER_S;
    if (strcmp(driver, DRIVER_STEEL_BATTALION) == 0)
        return DRIVER_STEEL_BATTALION;
    if(strcmp(driver, DRIVER_USB_PASSTHROUGH) == 0)
        return DRIVER_USB_PASSTHROUGH;
        
    return DRIVER_DUKE;
}

static void libusb_device_connected(LibusbDevice *device) {
    int p = xemu_input_get_libusb_device_default_bind_port(device, 0);
    assert(p >= -1 && p < 4);

    if(p >= 0 && !bound_libusb_devices[p] && !bound_controllers[p]) {
        bound_drivers[p] = DRIVER_USB_PASSTHROUGH;
        xemu_input_bind_passthrough(p, device, 1);

        char buf[128];
        snprintf(buf, sizeof(buf), "Connected '%s' to port %d", device->name, p+1);
        xemu_queue_notification(buf);
    }
}

static void libusb_device_disconnected(LibusbDevice *device) {
    if(device->bound >= 0) {
        char buf[128];
        snprintf(buf, 128, "Port %d Disconnected", device->bound + 1);
        xemu_queue_notification(buf);

        xemu_input_bind_passthrough(device->bound, NULL, 0);
    }
}

static const int port_map[4] = { 3, 4, 1, 2 };

void xemu_input_init(void)
{
    if (g_config.input.background_input_capture) {
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    }

    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        fprintf(stderr, "Failed to initialize SDL gamepad subsystem: %s\n", SDL_GetError());
        exit(1);
    }

    xemu_init_libusb_passthrough(libusb_device_connected, libusb_device_disconnected);

    // Create the keyboard input (always first)
    ControllerState *new_con = malloc(sizeof(ControllerState));
    memset(new_con, 0, sizeof(ControllerState));
    new_con->type = INPUT_DEVICE_SDL_KEYBOARD;
    new_con->name = "Keyboard";
    new_con->bound = -1;
    new_con->peripheral_types[0] = PERIPHERAL_NONE;
    new_con->peripheral_types[1] = PERIPHERAL_NONE;
    new_con->peripherals[0] = NULL;
    new_con->peripherals[1] = NULL;

    for (int i = 0; i < 25; i++) {
        static const char *format_str =
            "WARNING: Keyboard controller map scancode out of range "
            "(%d) : Disabled\n";
        char buf[128];
        snprintf(buf, sizeof(buf), format_str, i);
        check_and_reset_in_range(g_keyboard_scancode_map[i],
                                 SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_COUNT, buf);
    }
    
    sdl_sbc_kbd_scancode_map[3] =
        g_config.input.keyboard_sbc_scancode_map.eject;
    sdl_sbc_kbd_scancode_map[4] =
        g_config.input.keyboard_sbc_scancode_map.cockpit_hatch;
    sdl_sbc_kbd_scancode_map[5] =
        g_config.input.keyboard_sbc_scancode_map.ignition;
    sdl_sbc_kbd_scancode_map[6] =
        g_config.input.keyboard_sbc_scancode_map.start;
    sdl_sbc_kbd_scancode_map[7] =
        g_config.input.keyboard_sbc_scancode_map.open_close;
    sdl_sbc_kbd_scancode_map[8] =
        g_config.input.keyboard_sbc_scancode_map.map_zoom_in_out;
    sdl_sbc_kbd_scancode_map[9] =
        g_config.input.keyboard_sbc_scancode_map.mode_select;
    sdl_sbc_kbd_scancode_map[10] =
        g_config.input.keyboard_sbc_scancode_map.sub_monitor_mode_select;
    sdl_sbc_kbd_scancode_map[11] =
        g_config.input.keyboard_sbc_scancode_map.zoom_in;
    sdl_sbc_kbd_scancode_map[12] =
        g_config.input.keyboard_sbc_scancode_map.zoom_out;
    sdl_sbc_kbd_scancode_map[13] = g_config.input.keyboard_sbc_scancode_map.fss;
    sdl_sbc_kbd_scancode_map[14] =
        g_config.input.keyboard_sbc_scancode_map.manipulator;
    sdl_sbc_kbd_scancode_map[15] =
        g_config.input.keyboard_sbc_scancode_map.line_color_change;
    sdl_sbc_kbd_scancode_map[16] =
        g_config.input.keyboard_sbc_scancode_map.washing;
    sdl_sbc_kbd_scancode_map[17] =
        g_config.input.keyboard_sbc_scancode_map.extinguisher;
    sdl_sbc_kbd_scancode_map[18] =
        g_config.input.keyboard_sbc_scancode_map.chaff;
    sdl_sbc_kbd_scancode_map[19] =
        g_config.input.keyboard_sbc_scancode_map.tank_detach;
    sdl_sbc_kbd_scancode_map[20] =
        g_config.input.keyboard_sbc_scancode_map.override;
    sdl_sbc_kbd_scancode_map[21] =
        g_config.input.keyboard_sbc_scancode_map.night_scope;
    sdl_sbc_kbd_scancode_map[22] =
        g_config.input.keyboard_sbc_scancode_map.func1;
    sdl_sbc_kbd_scancode_map[23] =
        g_config.input.keyboard_sbc_scancode_map.func2;
    sdl_sbc_kbd_scancode_map[24] =
        g_config.input.keyboard_sbc_scancode_map.func3;
    sdl_sbc_kbd_scancode_map[25] =
        g_config.input.keyboard_sbc_scancode_map.main_weapon_control;
    sdl_sbc_kbd_scancode_map[26] =
        g_config.input.keyboard_sbc_scancode_map.sub_weapon_control;
    sdl_sbc_kbd_scancode_map[27] =
        g_config.input.keyboard_sbc_scancode_map.magazine_change;
    sdl_sbc_kbd_scancode_map[28] =
        g_config.input.keyboard_sbc_scancode_map.com1;
    sdl_sbc_kbd_scancode_map[29] =
        g_config.input.keyboard_sbc_scancode_map.com2;
    sdl_sbc_kbd_scancode_map[30] =
        g_config.input.keyboard_sbc_scancode_map.com3;
    sdl_sbc_kbd_scancode_map[31] =
        g_config.input.keyboard_sbc_scancode_map.com4;
    sdl_sbc_kbd_scancode_map[32] =
        g_config.input.keyboard_sbc_scancode_map.com5;
    sdl_sbc_kbd_scancode_map[33] =
        g_config.input.keyboard_sbc_scancode_map.sight_change;
    sdl_sbc_kbd_scancode_map[34] =
        g_config.input.keyboard_sbc_scancode_map.filt_control_system;
    sdl_sbc_kbd_scancode_map[35] =
        g_config.input.keyboard_sbc_scancode_map.oxygen_supply_system;
    sdl_sbc_kbd_scancode_map[36] =
        g_config.input.keyboard_sbc_scancode_map.fuel_flow_rate;
    sdl_sbc_kbd_scancode_map[37] =
        g_config.input.keyboard_sbc_scancode_map.buffer_material;
    sdl_sbc_kbd_scancode_map[38] =
        g_config.input.keyboard_sbc_scancode_map.vt_location_measurement;
    sdl_sbc_kbd_scancode_map[39] =
        g_config.input.keyboard_sbc_scancode_map.gear_up;
    sdl_sbc_kbd_scancode_map[40] =
        g_config.input.keyboard_sbc_scancode_map.gear_down;
    sdl_sbc_kbd_scancode_map[41] =
        g_config.input.keyboard_sbc_scancode_map.tuner_left;
    sdl_sbc_kbd_scancode_map[42] =
        g_config.input.keyboard_sbc_scancode_map.tuner_right;
    sdl_sbc_kbd_scancode_map[43] =
        g_config.input.keyboard_sbc_scancode_map.sight_change_up;
    sdl_sbc_kbd_scancode_map[44] =
        g_config.input.keyboard_sbc_scancode_map.sight_change_down;
    sdl_sbc_kbd_scancode_map[45] =
        g_config.input.keyboard_sbc_scancode_map.sight_change_left;
    sdl_sbc_kbd_scancode_map[46] =
        g_config.input.keyboard_sbc_scancode_map.sight_change_right;
    sdl_sbc_kbd_scancode_map[47] =
        g_config.input.keyboard_sbc_scancode_map.rotation_left;
    sdl_sbc_kbd_scancode_map[48] =
        g_config.input.keyboard_sbc_scancode_map.rotation_right;
    sdl_sbc_kbd_scancode_map[49] =
        g_config.input.keyboard_sbc_scancode_map.left_pedal;
    sdl_sbc_kbd_scancode_map[50] =
        g_config.input.keyboard_sbc_scancode_map.right_pedal;
    sdl_sbc_kbd_scancode_map[51] =
        g_config.input.keyboard_sbc_scancode_map.middle_pedal;

    for (int i = 0; i < 49; i++) {
        if ((sdl_sbc_kbd_scancode_map[i] < SDL_SCANCODE_UNKNOWN) ||
            (sdl_sbc_kbd_scancode_map[i] >= SDL_SCANCODE_COUNT)) {
            fprintf(stderr,
                    "WARNING: Keyboard steel battalion controller map scancode "
                    "out of range (%d) : Disabled\n",
                    sdl_sbc_kbd_scancode_map[i]);
            sdl_sbc_kbd_scancode_map[i] = SDL_SCANCODE_UNKNOWN;
        }
    }

    bound_drivers[0] = get_bound_driver(0);
    bound_drivers[1] = get_bound_driver(1);
    bound_drivers[2] = get_bound_driver(2);
    bound_drivers[3] = get_bound_driver(3);

    // Check to see if we should auto-bind the keyboard
    int port = xemu_input_get_controller_default_bind_port(new_con, 0);
    if (port >= 0) {
        xemu_input_bind(port, new_con, 0);
        char buf[128];
        snprintf(buf, sizeof(buf), "Connected '%s' to port %d", new_con->name, port+1);
        xemu_queue_notification(buf);
        xemu_input_rebind_xmu(port);
    }

    QTAILQ_INSERT_TAIL(&available_controllers, new_con, entry);
}

int xemu_input_get_controller_default_bind_port(ControllerState *state, int start)
{
    char guid[35] = { 0 };
    if (state->type == INPUT_DEVICE_SDL_GAMEPAD) {
        SDL_GUIDToString(state->sdl_joystick_guid, guid, sizeof(guid));
    } else if (state->type == INPUT_DEVICE_SDL_KEYBOARD) {
        snprintf(guid, sizeof(guid), "keyboard");
    }

    for (int i = start; i < 4; i++) {
        if (strcmp(guid, *port_index_to_settings_key_map[i]) == 0) {
            return i;
        }
    }

    return -1;
}

int xemu_input_get_libusb_device_default_bind_port(LibusbDevice *device, int start)
{
    assert(device);
    char guid[35] = { 0 };
    snprintf(guid, sizeof(guid), "USB\\%04x:%04x:%d:%s", device->vendor_id, device->product_id, device->host_bus, device->host_port);

    for(int i = start; i < 4; i++) {
        if(strcmp(guid, *port_index_to_settings_key_map[i])== 0) {
            return i;
        }
    }

    return -1;
}

void xemu_save_peripheral_settings(int player_index, int peripheral_index,
                                   int peripheral_type,
                                   const char *peripheral_parameter)
{
    int *peripheral_type_ptr =
        peripheral_types_settings_map[player_index][peripheral_index];
    const char **peripheral_param_ptr =
        peripheral_params_settings_map[player_index][peripheral_index];

    assert(peripheral_type_ptr);
    assert(peripheral_param_ptr);

    *peripheral_type_ptr = peripheral_type;
    xemu_settings_set_string(
        peripheral_param_ptr,
        peripheral_parameter == NULL ? "" : peripheral_parameter);
}

void xemu_input_process_sdl_events(const SDL_Event *event)
{
    if (event->type == SDL_EVENT_GAMEPAD_ADDED) {
        DPRINTF("Controller Added: %d\n", event->gdevice.which);

        // Attempt to open the added controller
        SDL_Gamepad *sdl_con;
        sdl_con = SDL_OpenGamepad(event->gdevice.which);
        if (sdl_con == NULL) {
            DPRINTF("Could not open joystick %d as a Gamepad\n", event->gdevice.which);
            return;
        }

        // Success! Create a new node to track this controller and continue init
        ControllerState *new_con = malloc(sizeof(ControllerState));
        memset(new_con, 0, sizeof(ControllerState));
        new_con->type                 = INPUT_DEVICE_SDL_GAMEPAD;
        new_con->name                 = SDL_GetGamepadName(sdl_con);
        new_con->sdl_gamepad          = sdl_con;
        new_con->sdl_joystick         = SDL_GetGamepadJoystick(new_con->sdl_gamepad);
        new_con->sdl_joystick_id      = SDL_GetJoystickID(new_con->sdl_joystick);
        new_con->sdl_joystick_guid    = SDL_GetJoystickGUID(new_con->sdl_joystick);
        new_con->bound                = -1;
        new_con->peripheral_types[0] = PERIPHERAL_NONE;
        new_con->peripheral_types[1] = PERIPHERAL_NONE;
        new_con->peripherals[0] = NULL;
        new_con->peripherals[1] = NULL;

        char guid_buf[35] = { 0 };
        SDL_GUIDToString(new_con->sdl_joystick_guid, guid_buf, sizeof(guid_buf));
        DPRINTF("Opened %s (%s)\n", new_con->name, guid_buf);

        QTAILQ_INSERT_TAIL(&available_controllers, new_con, entry);
        xemu_input_bindings_reload_map(new_con);

        // Do not replace binding for a currently bound device. In the case that
        // the same GUID is specified multiple times, on different ports, allow
        // any available port to be bound.
        //
        // This can happen naturally with X360 wireless receiver, in which each
        // controller gets the same GUID (go figure). We cannot remember which
        // controller is which in this case, but we can try to tolerate this
        // situation by binding to any previously bound port with this GUID. The
        // upside in this case is that a person can use the same GUID on all
        // ports and just needs to bind to the receiver and never needs to hit
        // this dialog.


        // Attempt to re-bind to port previously bound to
        int port = 0;
        bool did_bind = false;
        while (!did_bind) {
            port = xemu_input_get_controller_default_bind_port(new_con, port);
            if (port < 0) {
                // No (additional) default mappings
                break;
            } else if (!xemu_input_get_bound(port)) {
                xemu_input_bind(port, new_con, 0);
                did_bind = true;
                break;
            } else {
                // Try again for another port
                port++;
            }
        }

        // Try to bind to any open port, and if so remember the binding
        if (!did_bind && g_config.input.auto_bind) {
            for (port = 0; port < 4; port++) {
                if (!xemu_input_get_bound(port) &&
                    !xemu_input_get_bound_device(port)) {
                    xemu_input_bind(port, new_con, 1);
                    did_bind = true;
                    break;
                }
            }
        }

        if (did_bind) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Connected '%s' to port %d", new_con->name, port+1);
            xemu_queue_notification(buf);
            xemu_input_rebind_xmu(port);
        }
    } else if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
        DPRINTF("Controller Removed: %d\n", event->gdevice.which);
        int handled = 0;
        ControllerState *iter, *next;
        QTAILQ_FOREACH_SAFE(iter, &available_controllers, entry, next) {
            if (iter->type != INPUT_DEVICE_SDL_GAMEPAD) continue;

            if (iter->sdl_joystick_id == event->gdevice.which) {
                DPRINTF("Device removed: %s\n", iter->name);

                // Disconnect
                if (iter->bound >= 0) {
                    // Queue a notification to inform user controller disconnected
                    // FIXME: Probably replace with a callback registration thing,
                    // but this works well enough for now.
                    char buf[128];
                    snprintf(buf, sizeof(buf), "Port %d disconnected", iter->bound+1);
                    xemu_queue_notification(buf);

                    // Unbind the controller, but don't save the unbinding in
                    // case the controller is reconnected
                    xemu_input_bind(iter->bound, NULL, 0);
                }

                // Unlink
                QTAILQ_REMOVE(&available_controllers, iter, entry);

                // Deallocate
                if (iter->sdl_gamepad) {
                    SDL_CloseGamepad(iter->sdl_gamepad);
                }

                for (int i = 0; i < 2; i++) {
                    if (iter->peripherals[i])
                        g_free(iter->peripherals[i]);
                }
                free(iter);

                handled = 1;
                break;
            }
        }
        if (!handled) {
            DPRINTF("Could not find handle for joystick instance\n");
        }
    } else if (event->type == SDL_EVENT_GAMEPAD_REMAPPED) {
        DPRINTF("Controller Remapped: %d\n", event->gdevice.which);
    }
}

void xemu_input_update_controller(ControllerState *state)
{
    int64_t now = qemu_clock_get_us(QEMU_CLOCK_REALTIME);
    if (ABS(now - state->last_input_updated_ts) <
        XEMU_INPUT_MIN_INPUT_UPDATE_INTERVAL_US) {
        return;
    }

    if (state->type == INPUT_DEVICE_SDL_KEYBOARD) {
        xemu_input_update_sdl_kbd_controller_state(state);
    } else if (state->type == INPUT_DEVICE_SDL_GAMEPAD) {
        xemu_input_update_sdl_controller_state(state);
    }

    state->last_input_updated_ts = qemu_clock_get_us(QEMU_CLOCK_REALTIME);
}

void xemu_input_update_controllers(void)
{
    ControllerState *iter;
    QTAILQ_FOREACH(iter, &available_controllers, entry) {
        xemu_input_update_controller(iter);
    }
    QTAILQ_FOREACH(iter, &available_controllers, entry) {
        xemu_input_update_rumble(iter);
    }
}

void xemu_input_update_sdl_kbd_controller_state(ControllerState *state)
{
    state->gp.buttons = 0;
    state->sbc.buttons = 0;
    memset(state->gp.axis, 0, sizeof(state->gp.axis));
    memset(state->sbc.axis, 0, sizeof(state->sbc.axis));

    const bool *kbd = SDL_GetKeyboardState(NULL);

    if (state->bound < 0)
        return;

    const char *bound_driver = get_bound_driver(state->bound);
    if (strcmp(bound_driver, DRIVER_STEEL_BATTALION) == 0) {

        if (state->sbc.gearLever == 0)
            state->sbc.gearLever = 255;

        // Update SBC Buttons
        for (int i = 3; i < 43; i++) {
            if (kbd[sdl_sbc_kbd_scancode_map[i]])
                state->sbc.buttons |= (1ULL << i);
        }

        const uint64_t toggles[5] = { SBC_BUTTON_FILT_CONTROL_SYSTEM,
                                      SBC_BUTTON_OXYGEN_SUPPLY_SYSTEM,
                                      SBC_BUTTON_FUEL_FLOW_RATE,
                                      SBC_BUTTON_BUFFER_MATERIAL,
                                      SBC_BUTTON_VT_LOCATION_MEASUREMENT };

        for (int i = 0; i < 5; i++) {
            if ((state->sbc.buttons & toggles[i]) &&
                !(state->sbc.previousButtons &
                  toggles[i])) { // When the for the toggle is pressed
                uint8_t byteMask = (uint8_t)(toggles[i] >> 32);
                // Toggle the toggle switch
                state->sbc.toggleSwitches ^= byteMask;
            }
        }

        // Tuner Dial Left
        if ((state->sbc.buttons & SBC_BUTTON_TUNER_LEFT) &&
            !(state->sbc.previousButtons & SBC_BUTTON_TUNER_LEFT)) {
            if (state->sbc.tunerDial == 0)
                state->sbc.tunerDial = 15;
            else
                state->sbc.tunerDial--;
        }

        // Tuner Dial Right
        if ((state->sbc.buttons & SBC_BUTTON_TUNER_RIGHT) &&
            !(state->sbc.previousButtons & SBC_BUTTON_TUNER_RIGHT)) {
            if (state->sbc.tunerDial == 15)
                state->sbc.tunerDial = 0;
            else
                state->sbc.tunerDial++;
        }

        // Gear Lever Up
        if ((state->sbc.buttons & SBC_BUTTON_GEAR_UP) &&
            !(state->sbc.previousButtons & SBC_BUTTON_GEAR_UP)) {
            if (state->sbc.gearLever != 5) {
                if (state->sbc.gearLever == 255)
                    state->sbc.gearLever = 1;
                else
                    state->sbc.gearLever++;
            }
        }

        // Gear Lever Down
        if ((state->sbc.buttons & SBC_BUTTON_GEAR_DOWN) &&
            !(state->sbc.previousButtons & SBC_BUTTON_GEAR_DOWN)) {
            if (state->sbc.gearLever != 254) {
                if (state->sbc.gearLever == 1)
                    state->sbc.gearLever = 255;
                else
                    state->sbc.gearLever--;
            }
        }

        // Update SBC Axes
        float mouseX, mouseY;
        uint32_t mouseBtn = SDL_GetMouseState(&mouseX, &mouseY);

        if (mouseBtn & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) {
            state->sbc.buttons |= SBC_BUTTON_MAIN_WEAPON;
        }
        if (mouseBtn & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) {
            state->sbc.buttons |= SBC_BUTTON_LOCK_ON;
        }
        if(mouseBtn & SDL_BUTTON_MASK(SDL_BUTTON_X1) ||
           mouseBtn & SDL_BUTTON_MASK(SDL_BUTTON_X2)) {
            state->sbc.buttons |= SBC_BUTTON_SUB_WEAPON;
        }

        int32_t windowWidth, windowHeight;
        SDL_GetWindowSizeInPixels(
            m_window, &windowWidth,
            &windowHeight); // get the mouse location relative to the Viewport,
                            // not the Window

        // Calculate the position of the mouse coordinates in [-32768,32768]
        DPRINTF("[Steel Battalion] Window Coordinates: %d, %d\n", mouseX, mouseY);

        // Check that the mouse position is within the window coordinates
        if (mouseX >= 0 && mouseX <= windowWidth && mouseY >= 0 &&
            mouseY <= windowHeight) {
            if (viewport_coords[2] > 0 && viewport_coords[3] > 0) {
                // Switch from Window coordinates to Viewport Coordinates
                mouseX -= viewport_coords[0];
                mouseY -= viewport_coords[1];
                windowWidth = viewport_coords[2];
                windowHeight = viewport_coords[3];
            }

            DPRINTF("[Steel Battalion] Viewport Coordinates: %d, %d\n", mouseX, mouseY);
            int32_t x = (int32_t)((mouseX - (windowWidth / 2)) *
                                  65535 / windowWidth);
            int32_t y = (int32_t)((mouseY - (windowHeight / 2)) *
                                  65535 / windowHeight);

            state->sbc.axis[SBC_AXIS_AIMING_X] = 
                (int16_t)MIN(MAX(x, -32768), 32767);
            state->sbc.axis[SBC_AXIS_AIMING_Y] = 
                (int16_t)MIN(MAX(y, -32768), 32767);

            DPRINTF("[Steel Battalion] X: %d, Y: %d", 
                    state->sbc.axis[SBC_AXIS_AIMING_X], 
                    state->sbc.axis[SBC_AXIS_AIMING_Y]);
        }

        if (kbd[sdl_sbc_kbd_scancode_map[43]])
            state->sbc.axis[SBC_AXIS_SIGHT_CHANGE_Y] = -32768;
        if (kbd[sdl_sbc_kbd_scancode_map[44]])
            state->sbc.axis[SBC_AXIS_SIGHT_CHANGE_Y] = 32767;
        if (kbd[sdl_sbc_kbd_scancode_map[45]])
            state->sbc.axis[SBC_AXIS_SIGHT_CHANGE_X] = -32768;
        if (kbd[sdl_sbc_kbd_scancode_map[46]])
            state->sbc.axis[SBC_AXIS_SIGHT_CHANGE_X] = 32767;

        if (kbd[sdl_sbc_kbd_scancode_map[47]])
            state->sbc.axis[SBC_AXIS_ROTATION_LEVER] = -32768;
        if (kbd[sdl_sbc_kbd_scancode_map[48]])
            state->sbc.axis[SBC_AXIS_ROTATION_LEVER] = 32767;

        if (kbd[sdl_sbc_kbd_scancode_map[49]])
            state->sbc.axis[SBC_AXIS_LEFT_PEDAL] = SBC_PEDAL_MAX;
        if (kbd[sdl_sbc_kbd_scancode_map[50]])
            state->sbc.axis[SBC_AXIS_RIGHT_PEDAL] = SBC_PEDAL_MAX;
        if (kbd[sdl_sbc_kbd_scancode_map[51]])
            state->sbc.axis[SBC_AXIS_MIDDLE_PEDAL] = SBC_PEDAL_MAX;

        state->sbc.previousButtons = state->sbc.buttons;
    } else {

#define KBD_STATE(btn) \
        (kbd[g_config.input.keyboard_controller_scancode_map.btn])

        state->gp.buttons |= KBD_STATE(a) << 0;
        state->gp.buttons |= KBD_STATE(b) << 1;
        state->gp.buttons |= KBD_STATE(x) << 2;
        state->gp.buttons |= KBD_STATE(y) << 3;
        state->gp.buttons |= KBD_STATE(dpad_left) << 4;
        state->gp.buttons |= KBD_STATE(dpad_up) << 5;
        state->gp.buttons |= KBD_STATE(dpad_right) << 6;
        state->gp.buttons |= KBD_STATE(dpad_down) << 7;
        state->gp.buttons |= KBD_STATE(back) << 8;
        state->gp.buttons |= KBD_STATE(start) << 9;
        state->gp.buttons |= KBD_STATE(white) << 10;
        state->gp.buttons |= KBD_STATE(black) << 11;
        state->gp.buttons |= KBD_STATE(lstick_btn) << 12;
        state->gp.buttons |= KBD_STATE(rstick_btn) << 13;
        state->gp.buttons |= KBD_STATE(guide) << 14;

        if (KBD_STATE(lstick_up))
            state->gp.axis[CONTROLLER_AXIS_LSTICK_Y] = 32767;
        if (KBD_STATE(lstick_left))
            state->gp.axis[CONTROLLER_AXIS_LSTICK_X] = -32768;
        if (KBD_STATE(lstick_right))
            state->gp.axis[CONTROLLER_AXIS_LSTICK_X] = 32767;
        if (KBD_STATE(lstick_down))
            state->gp.axis[CONTROLLER_AXIS_LSTICK_Y] = -32768;
        if (KBD_STATE(ltrigger))
            state->gp.axis[CONTROLLER_AXIS_LTRIG] = 32767;

        if (KBD_STATE(rstick_up))
            state->gp.axis[CONTROLLER_AXIS_RSTICK_Y] = 32767;
        if (KBD_STATE(rstick_left))
            state->gp.axis[CONTROLLER_AXIS_RSTICK_X] = -32768;
        if (KBD_STATE(rstick_right))
            state->gp.axis[CONTROLLER_AXIS_RSTICK_X] = 32767;
        if (KBD_STATE(rstick_down))
            state->gp.axis[CONTROLLER_AXIS_RSTICK_Y] = -32768;
        if (KBD_STATE(rtrigger))
            state->gp.axis[CONTROLLER_AXIS_RTRIG] = 32767;
#undef KBD_STATE
    }
}

void xemu_input_update_sdl_controller_state(ControllerState *state)
{
    state->gp.buttons = 0;
    state->sbc.buttons = 0;
    memset(state->gp.axis, 0, sizeof(state->gp.axis));
    memset(state->sbc.axis, 0, sizeof(state->sbc.axis));

    if (state->bound < 0)
        return;

    const char *bound_driver = get_bound_driver(state->bound);
    if (strcmp(bound_driver, DRIVER_STEEL_BATTALION) == 0) {
        state->sbc.buttons = 0;

        const uint64_t sdl_button_map_sbc[43][2] = {
            { g_config.input.sbc_binding_map.main_weapon, SBC_BUTTON_MAIN_WEAPON },
            { g_config.input.sbc_binding_map.lock_on, SBC_BUTTON_LOCK_ON},
            { g_config.input.sbc_binding_map.sub_weapon, SBC_BUTTON_SUB_WEAPON },
            { g_config.input.sbc_binding_map.eject, SBC_BUTTON_EJECT },
            { g_config.input.sbc_binding_map.cockpit_hatch, SBC_BUTTON_COCKPIT_HATCH },
            { g_config.input.sbc_binding_map.ignition, SBC_BUTTON_IGNITION },
            { g_config.input.sbc_binding_map.start, SBC_BUTTON_START },
            { g_config.input.sbc_binding_map.open_close, SBC_BUTTON_OPEN_CLOSE },
            { g_config.input.sbc_binding_map.map_zoom_in_out, SBC_BUTTON_MAP_ZOOM_IN_OUT },
            { g_config.input.sbc_binding_map.mode_select, SBC_BUTTON_MODE_SELECT },
            { g_config.input.sbc_binding_map.sub_monitor_mode_select, SBC_BUTTON_SUB_MONITOR_MODE_SELECT },
            { g_config.input.sbc_binding_map.zoom_in, SBC_BUTTON_ZOOM_IN },
            { g_config.input.sbc_binding_map.zoom_out, SBC_BUTTON_ZOOM_OUT },
            { g_config.input.sbc_binding_map.fss, SBC_BUTTON_FSS },
            { g_config.input.sbc_binding_map.manipulator, SBC_BUTTON_MANIPULATOR },
            { g_config.input.sbc_binding_map.line_color_change, SBC_BUTTON_LINE_COLOR_CHANGE },
            { g_config.input.sbc_binding_map.washing, SBC_BUTTON_WASHING },
            { g_config.input.sbc_binding_map.extinguisher, SBC_BUTTON_EXTINGUISHER },
            { g_config.input.sbc_binding_map.chaff, SBC_BUTTON_CHAFF },
            { g_config.input.sbc_binding_map.tank_detach, SBC_BUTTON_TANK_DETACH },
            { g_config.input.sbc_binding_map.override, SBC_BUTTON_OVERRIDE },
            { g_config.input.sbc_binding_map.night_scope, SBC_BUTTON_NIGHT_SCOPE },
            { g_config.input.sbc_binding_map.func1, SBC_BUTTON_FUNC1 },
            { g_config.input.sbc_binding_map.func2, SBC_BUTTON_FUNC2 },
            { g_config.input.sbc_binding_map.func3, SBC_BUTTON_FUNC3 },
            { g_config.input.sbc_binding_map.main_weapon_control, SBC_BUTTON_MAIN_WEAPON_CONTROL },
            { g_config.input.sbc_binding_map.sub_weapon_control, SBC_BUTTON_SUB_WEAPON_CONTROL },
            { g_config.input.sbc_binding_map.magazine_change, SBC_BUTTON_MAGAZINE_CHANGE },
            { g_config.input.sbc_binding_map.com1, SBC_BUTTON_COM1 },
            { g_config.input.sbc_binding_map.com2, SBC_BUTTON_COM2 },
            { g_config.input.sbc_binding_map.com3, SBC_BUTTON_COM3 },
            { g_config.input.sbc_binding_map.com4, SBC_BUTTON_COM4 },
            { g_config.input.sbc_binding_map.com5, SBC_BUTTON_COM5 },
            { g_config.input.sbc_binding_map.sight_change, SBC_BUTTON_SIGHT_CHANGE },
            { g_config.input.sbc_binding_map.filt_control_system, SBC_BUTTON_FILT_CONTROL_SYSTEM },
            { g_config.input.sbc_binding_map.oxygen_supply_system, SBC_BUTTON_OXYGEN_SUPPLY_SYSTEM },
            { g_config.input.sbc_binding_map.fuel_flow_rate, SBC_BUTTON_FUEL_FLOW_RATE },
            { g_config.input.sbc_binding_map.buffer_material, SBC_BUTTON_BUFFER_MATERIAL },
            { g_config.input.sbc_binding_map.vt_location_measurement, SBC_BUTTON_VT_LOCATION_MEASUREMENT },
            { g_config.input.sbc_binding_map.gear_up, SBC_BUTTON_GEAR_UP },
            { g_config.input.sbc_binding_map.gear_down, SBC_BUTTON_GEAR_DOWN },
            { g_config.input.sbc_binding_map.tuner_left, SBC_BUTTON_TUNER_LEFT },
            { g_config.input.sbc_binding_map.tuner_right, SBC_BUTTON_TUNER_RIGHT }
        };

        if (state->sbc.gearLever == 0)
            state->sbc.gearLever = 255;

        for (int i = 0; i < 43; i++) {
            if(sdl_button_map_sbc[i][0] != -1) {
                if (SDL_GetGamepadButton(state->sdl_gamepad,
                                                sdl_button_map_sbc[i][0]))
                    state->sbc.buttons |= sdl_button_map_sbc[i][1];
            }
        }

        const uint64_t toggles[5] = { SBC_BUTTON_FILT_CONTROL_SYSTEM,
                                      SBC_BUTTON_OXYGEN_SUPPLY_SYSTEM,
                                      SBC_BUTTON_FUEL_FLOW_RATE,
                                      SBC_BUTTON_BUFFER_MATERIAL,
                                      SBC_BUTTON_VT_LOCATION_MEASUREMENT };

        for (int i = 0; i < 5; i++) {
            if ((state->sbc.buttons & toggles[i]) &&
                !(state->sbc.previousButtons &
                  toggles[i])) { // When the for the toggle is pressed
                uint8_t byteMask = (uint8_t)(toggles[i] >> 32);
                // Toggle the toggle switch
                state->sbc.toggleSwitches ^= byteMask;
            }
        }

        // Tuner Dial Left
        if ((state->sbc.buttons & SBC_BUTTON_TUNER_LEFT) &&
            !(state->sbc.previousButtons & SBC_BUTTON_TUNER_LEFT)) {
            if (state->sbc.tunerDial == 0)
                state->sbc.tunerDial = 15;
            else
                state->sbc.tunerDial--;
        }

        // Tuner Dial Right
        if ((state->sbc.buttons & SBC_BUTTON_TUNER_RIGHT) &&
            !(state->sbc.previousButtons & SBC_BUTTON_TUNER_RIGHT)) {
            if (state->sbc.tunerDial == 15)
                state->sbc.tunerDial = 0;
            else
                state->sbc.tunerDial++;
        }

        // Gear Lever Up
        if ((state->sbc.buttons & SBC_BUTTON_GEAR_UP) &&
            !(state->sbc.previousButtons & SBC_BUTTON_GEAR_UP)) {
            if (state->sbc.gearLever != 5) {
                if (state->sbc.gearLever == 255)
                    state->sbc.gearLever = 1;
                else
                    state->sbc.gearLever++;
            }
        }

        // Gear Lever Down
        if ((state->sbc.buttons & SBC_BUTTON_GEAR_DOWN) &&
            !(state->sbc.previousButtons & SBC_BUTTON_GEAR_DOWN)) {
            if (state->sbc.gearLever != 254) {
                if (state->sbc.gearLever == 1)
                    state->sbc.gearLever = 255;
                else
                    state->sbc.gearLever--;
            }
        }
#define SDL_GET_AXIS(state, axis)    \
    SDL_GetGamepadAxis(              \
        (state)->sdl_gamepad, axis)

        int sdl_axis_map_sbc[SBC_AXIS__COUNT] = {
            g_config.input.sbc_binding_map.aiming_x,
            g_config.input.sbc_binding_map.aiming_y,
            g_config.input.sbc_binding_map.rotation,
            g_config.input.sbc_binding_map.left_pedal,
            g_config.input.sbc_binding_map.middle_pedal,
            g_config.input.sbc_binding_map.right_pedal,
            g_config.input.sbc_binding_map.sight_change_x,
            g_config.input.sbc_binding_map.sight_change_y
        };

        for(int i = 0; i < SBC_AXIS__COUNT; i++) {
            if(sdl_axis_map_sbc[i] != -1) {
                state->sbc.axis[i] = SDL_GetGamepadAxis(
                    state->sdl_gamepad, sdl_axis_map_sbc[i]);
            }
        }
        
        state->sbc.previousButtons = state->sbc.buttons;
    } else {
#define SDL_MASK_BUTTON(state, btn, idx)                  \
    (SDL_GetGamepadButton(                                \
         (state)->sdl_gamepad,                            \
         (state)->controller_map->controller_mapping.btn) \
     << idx)

        state->gp.buttons |= SDL_MASK_BUTTON(state, a, 0);
        state->gp.buttons |= SDL_MASK_BUTTON(state, b, 1);
        state->gp.buttons |= SDL_MASK_BUTTON(state, x, 2);
        state->gp.buttons |= SDL_MASK_BUTTON(state, y, 3);
        state->gp.buttons |= SDL_MASK_BUTTON(state, dpad_left, 4);
        state->gp.buttons |= SDL_MASK_BUTTON(state, dpad_up, 5);
        state->gp.buttons |= SDL_MASK_BUTTON(state, dpad_right, 6);
        state->gp.buttons |= SDL_MASK_BUTTON(state, dpad_down, 7);
        state->gp.buttons |= SDL_MASK_BUTTON(state, back, 8);
        state->gp.buttons |= SDL_MASK_BUTTON(state, start, 9);
        state->gp.buttons |= SDL_MASK_BUTTON(state, lshoulder, 10);
        state->gp.buttons |= SDL_MASK_BUTTON(state, rshoulder, 11);
        state->gp.buttons |= SDL_MASK_BUTTON(state, lstick_btn, 12);
        state->gp.buttons |= SDL_MASK_BUTTON(state, rstick_btn, 13);
        state->gp.buttons |= SDL_MASK_BUTTON(state, guide, 14);
        state->gp.buttons |= SDL_MASK_BUTTON(state, a, 0);
        state->gp.buttons |= SDL_MASK_BUTTON(state, b, 1);
        state->gp.buttons |= SDL_MASK_BUTTON(state, x, 2);
        state->gp.buttons |= SDL_MASK_BUTTON(state, y, 3);
        state->gp.buttons |= SDL_MASK_BUTTON(state, dpad_left, 4);
        state->gp.buttons |= SDL_MASK_BUTTON(state, dpad_up, 5);
        state->gp.buttons |= SDL_MASK_BUTTON(state, dpad_right, 6);
        state->gp.buttons |= SDL_MASK_BUTTON(state, dpad_down, 7);
        state->gp.buttons |= SDL_MASK_BUTTON(state, back, 8);
        state->gp.buttons |= SDL_MASK_BUTTON(state, start, 9);
        state->gp.buttons |= SDL_MASK_BUTTON(state, lshoulder, 10);
        state->gp.buttons |= SDL_MASK_BUTTON(state, rshoulder, 11);
        state->gp.buttons |= SDL_MASK_BUTTON(state, lstick_btn, 12);
        state->gp.buttons |= SDL_MASK_BUTTON(state, rstick_btn, 13);
        state->gp.buttons |= SDL_MASK_BUTTON(state, guide, 14);

    #undef SDL_MASK_BUTTON

#define SDL_GET_AXIS(state, axis)    \
    SDL_GetGamepadAxis(              \
        (state)->sdl_gamepad,        \
        (state)->controller_map->controller_mapping.axis)

        state->gp.axis[0] = SDL_GET_AXIS(state, axis_trigger_left);
        state->gp.axis[1] = SDL_GET_AXIS(state, axis_trigger_right);
        state->gp.axis[2] = SDL_GET_AXIS(state, axis_left_x);
        state->gp.axis[3] = SDL_GET_AXIS(state, axis_left_y);
        state->gp.axis[4] = SDL_GET_AXIS(state, axis_right_x);
        state->gp.axis[5] = SDL_GET_AXIS(state, axis_right_y);
        state->gp.axis[0] = SDL_GET_AXIS(state, axis_trigger_left);
        state->gp.axis[1] = SDL_GET_AXIS(state, axis_trigger_right);
        state->gp.axis[2] = SDL_GET_AXIS(state, axis_left_x);
        state->gp.axis[3] = SDL_GET_AXIS(state, axis_left_y);
        state->gp.axis[4] = SDL_GET_AXIS(state, axis_right_x);
        state->gp.axis[5] = SDL_GET_AXIS(state, axis_right_y);

#undef SDL_GET_AXIS

// FIXME: Check range
#define INVERT_AXIS(controller_axis) \
    state->gp.axis[controller_axis] = -1 - state->gp.axis[controller_axis]

        if (state->controller_map->controller_mapping.invert_axis_left_x) {
            INVERT_AXIS(CONTROLLER_AXIS_LSTICK_X);
        }
        if (state->controller_map->controller_mapping.invert_axis_left_x) {
            INVERT_AXIS(CONTROLLER_AXIS_LSTICK_X);
        }

        if (!state->controller_map->controller_mapping.invert_axis_left_y) {
            INVERT_AXIS(CONTROLLER_AXIS_LSTICK_Y);
        }
        if (!state->controller_map->controller_mapping.invert_axis_left_y) {
            INVERT_AXIS(CONTROLLER_AXIS_LSTICK_Y);
        }

        if (state->controller_map->controller_mapping.invert_axis_right_x) {
            INVERT_AXIS(CONTROLLER_AXIS_RSTICK_X);
        }
        if (state->controller_map->controller_mapping.invert_axis_right_x) {
            INVERT_AXIS(CONTROLLER_AXIS_RSTICK_X);
        }

        if (!state->controller_map->controller_mapping.invert_axis_right_y) {
            INVERT_AXIS(CONTROLLER_AXIS_RSTICK_Y);
        }
        if (!state->controller_map->controller_mapping.invert_axis_right_y) {
            INVERT_AXIS(CONTROLLER_AXIS_RSTICK_Y);
        }

#undef INVERT_AXIS

    // xemu_input_print_controller_state(state);
    }
}

void xemu_input_update_rumble(ControllerState *state)
{
    if (state->type != INPUT_DEVICE_SDL_GAMEPAD) {
        return;
    }

    if (!state->controller_map->enable_rumble) {
        return;
    }

    int64_t now = qemu_clock_get_us(QEMU_CLOCK_REALTIME);
    if (ABS(now - state->last_rumble_updated_ts) <
        XEMU_INPUT_MIN_RUMBLE_UPDATE_INTERVAL_US) {
        return;
    }

    SDL_RumbleGamepad(state->sdl_gamepad, state->gp.rumble_l, state->gp.rumble_r, 250);
    SDL_RumbleGamepad(state->sdl_gamepad, state->gp.rumble_l, state->gp.rumble_r, 250);
    state->last_rumble_updated_ts = qemu_clock_get_us(QEMU_CLOCK_REALTIME);
}

ControllerState *xemu_input_get_bound(int index)
{
    return bound_controllers[index];
}

LibusbDevice *xemu_input_get_bound_device(int index)
{
    return bound_libusb_devices[index];
}

static DeviceState *xemu_bind_usb_hub(int num_ports, const char *port)
{
    QDict *usbhub_qdict = qdict_new();
    qdict_put_str(usbhub_qdict, "driver", "usb-hub");
    qdict_put_str(usbhub_qdict, "port", port);
    qdict_put_int(usbhub_qdict, "ports", num_ports);
    QemuOpts *usbhub_opts = qemu_opts_from_qdict(qemu_find_opts("device"),
                                                 usbhub_qdict, &error_abort);
    DeviceState *usbhub_dev = qdev_device_add(usbhub_opts, &error_abort);

    return usbhub_dev;
}

static DeviceState *xemu_input_bind_driver(int index, const char *port,
                                           const char *driver)
{
    // Create XID controller. This is connected to Port 1 of the controller's
    // internal USB Hub
    QDict *qdict = qdict_new();

    // Specify device driver
    qdict_put_str(qdict, "driver", driver);

    // Specify device identifier
    static int id_counter = 0;
    char *tmp = g_strdup_printf("gamepad_%d", id_counter++);
    qdict_put_str(qdict, "id", tmp);
    g_free(tmp);

    // Specify index/port
    qdict_put_int(qdict, "index", index);
    qdict_put_str(qdict, "port", port);

    // Create the device
    QemuOpts *opts =
        qemu_opts_from_qdict(qemu_find_opts("device"), qdict, &error_abort);
    DeviceState *dev = qdev_device_add(opts, &error_abort);
    assert(dev);

    qobject_unref(qdict);
    return dev;
}

static DeviceState *xemu_bind_usb_host(int hostbus, const char *hostport,
                                       const char *port)
{
    // Create XID controller. This is connected to Port 1 of the controller's
    // internal USB Hub
    QDict *qdict = qdict_new();

    // specify device driver
    qdict_put_str(qdict, "driver", "usb-host");

    // specify hostbus
    qdict_put_int(qdict, "hostbus", hostbus);

    // specify hostport
    qdict_put_str(qdict, "hostport", hostport);

    // Specify port
    qdict_put_str(qdict, "port", port);

    // Create the device
    QemuOpts *opts =
        qemu_opts_from_qdict(qemu_find_opts("device"), qdict, &error_abort);
    DeviceState *dev = qdev_device_add(opts, &error_abort);
    assert(dev);

    qobject_unref(qdict);

    return dev;
}

void xemu_input_bind(int index, ControllerState *state, int save)
{
    // FIXME: Attempt to disable rumble when unbinding so it's not left
    // in rumble mode

    // Unbind existing controller
    if (bound_controllers[index]) {
        assert(bound_controllers[index]->device != NULL);
        Error *err = NULL;

        // Unbind any XMUs
        for (int i = 0; i < 2; i++) {
            if (bound_controllers[index]->peripherals[i]) {
                // If this was an XMU, unbind the XMU
                if (bound_controllers[index]->peripheral_types[i] ==
                    PERIPHERAL_XMU)
                    xemu_input_unbind_xmu(index, i);

                // Free up the XmuState and set the peripheral type to none
                g_free(bound_controllers[index]->peripherals[i]);
                bound_controllers[index]->peripherals[i] = NULL;
                bound_controllers[index]->peripheral_types[i] = PERIPHERAL_NONE;
            }
        }

        qdev_unplug((DeviceState *)bound_controllers[index]->device, &err);
        assert(err == NULL);

        bound_controllers[index]->bound = -1;
        bound_controllers[index]->device = NULL;
        bound_controllers[index] = NULL;
    }

    // Save this controller's GUID in settings for auto re-connect
    if (save) {
        char guid_buf[35] = { 0 };
        if (state) {
            if (state->type == INPUT_DEVICE_SDL_GAMEPAD) {
                SDL_GUIDToString(state->sdl_joystick_guid, guid_buf, sizeof(guid_buf));
            } else if (state->type == INPUT_DEVICE_SDL_KEYBOARD) {
                snprintf(guid_buf, sizeof(guid_buf), "keyboard");
            }
        }
        xemu_settings_set_string(port_index_to_settings_key_map[index], guid_buf);
        xemu_settings_set_string(port_index_to_driver_settings_key_map[index],
                                 bound_drivers[index]);
    }

    if (strcmp(bound_drivers[index], DRIVER_USB_PASSTHROUGH) != 0) {
        if (state) {
            if (state->bound >= 0) {
                // Device was already bound to another port. Unbind it.
                xemu_input_bind(state->bound, NULL, 1);
            }

            bound_controllers[index] = state;
            bound_controllers[index]->bound = index;

            char *tmp;

            bool hasInternalHub =
                strcmp(bound_drivers[index], DRIVER_STEEL_BATTALION) != 0;
            char *port;
            DeviceState *dev;

            if (hasInternalHub) {
                // Create controller's internal USB hub.
                tmp = g_strdup_printf("1.%d", port_map[index]);
                dev = xemu_bind_usb_hub(3, tmp);
                g_free(tmp);
                port = g_strdup_printf("1.%d.1", port_map[index]);
                DeviceState *tmp_dev =
                    xemu_input_bind_driver(index, port, bound_drivers[index]);
                object_unref(OBJECT(tmp_dev));
            } else {
                port = g_strdup_printf("1.%d", port_map[index]);
                dev = xemu_input_bind_driver(index, port, bound_drivers[index]);
            }
            g_free(port);

            // Unref for eventual cleanup
            object_unref(OBJECT(dev));

            state->device = dev;
        }
    }
}

bool xemu_input_bind_xmu(int player_index, int expansion_slot_index,
                         const char *filename, bool is_rebind)
{
    assert(player_index >= 0 && player_index < 4);
    assert(expansion_slot_index >= 0 && expansion_slot_index < 2);

    bool hasInternalHub =
        strcmp(bound_drivers[player_index], DRIVER_STEEL_BATTALION) != 0;
    assert(hasInternalHub);

    ControllerState *player = bound_controllers[player_index];
    enum peripheral_type peripheral_type =
        player->peripheral_types[expansion_slot_index];
    if (peripheral_type != PERIPHERAL_XMU)
        return false;

    XmuState *xmu = (XmuState *)player->peripherals[expansion_slot_index];

    // Unbind existing XMU
    if (xmu->dev != NULL) {
        xemu_input_unbind_xmu(player_index, expansion_slot_index);
    }

    if (filename == NULL)
        return false;

    // Look for any other XMUs that are using this file, and unbind them
    for (int player_i = 0; player_i < 4; player_i++) {
        ControllerState *state = bound_controllers[player_i];
        if (state != NULL) {
            for (int peripheral_i = 0; peripheral_i < 2; peripheral_i++) {
                if (state->peripheral_types[peripheral_i] == PERIPHERAL_XMU) {
                    XmuState *xmu_i =
                        (XmuState *)state->peripherals[peripheral_i];
                    assert(xmu_i);

                    if (xmu_i->filename != NULL &&
                        strcmp(xmu_i->filename, filename) == 0) {
                        char *buf =
                            g_strdup_printf("This XMU is already mounted on "
                                            "player %d slot %c\r\n",
                                            player_i + 1, 'A' + peripheral_i);
                        xemu_queue_notification(buf);
                        g_free(buf);
                        return false;
                    }
                }
            }
        }
    }

    xmu->filename = g_strdup(filename);

    const int xmu_map[2] = { 2, 3 };
    char *tmp;

    static int id_counter = 0;
    tmp = g_strdup_printf("xmu_%d", id_counter++);

    // Add the file as a drive
    QDict *qdict1 = qdict_new();
    qdict_put_str(qdict1, "id", tmp);
    qdict_put_str(qdict1, "format", "raw");
    qdict_put_str(qdict1, "file", filename);

    QemuOpts *drvopts =
        qemu_opts_from_qdict(qemu_find_opts("drive"), qdict1, &error_abort);

    DriveInfo *dinfo = drive_new(drvopts, 0, &error_abort);
    assert(dinfo);

    // Create the usb-storage device
    QDict *qdict2 = qdict_new();

    // Specify device driver
    qdict_put_str(qdict2, "driver", "usb-storage");

    // Specify device identifier
    qdict_put_str(qdict2, "drive", tmp);
    g_free(tmp);

    // Specify index/port
    tmp = g_strdup_printf("1.%d.%d", port_map[player_index],
                          xmu_map[expansion_slot_index]);
    qdict_put_str(qdict2, "port", tmp);
    g_free(tmp);

    // Create the device
    QemuOpts *opts =
        qemu_opts_from_qdict(qemu_find_opts("device"), qdict2, &error_abort);

    DeviceState *dev = qdev_device_add(opts, &error_abort);
    assert(dev);

    xmu->dev = (void *)dev;

    // Unref for eventual cleanup
    qobject_unref(qdict1);
    qobject_unref(qdict2);

    if (!is_rebind) {
        xemu_save_peripheral_settings(player_index, expansion_slot_index,
                                      peripheral_type, xmu->filename);
    }

    return true;
}

void xemu_input_unbind_xmu(int player_index, int expansion_slot_index)
{
    assert(player_index >= 0 && player_index < 4);
    assert(expansion_slot_index >= 0 && expansion_slot_index < 2);

    ControllerState *state = bound_controllers[player_index];
    if (state->peripheral_types[expansion_slot_index] != PERIPHERAL_XMU)
        return;

    XmuState *xmu = (XmuState *)state->peripherals[expansion_slot_index];
    if (xmu != NULL) {
        if (xmu->dev != NULL) {
            qdev_unplug((DeviceState *)xmu->dev, &error_abort);
            object_unref(OBJECT(xmu->dev));
            xmu->dev = NULL;
        }

        g_free((void *)xmu->filename);
        xmu->filename = NULL;
    }
}

void xemu_input_rebind_xmu(int port)
{
    bool hasInternalHub =
        strcmp(bound_drivers[port], DRIVER_STEEL_BATTALION) != 0;
    if (!hasInternalHub)
        return;
        
    // Try to bind peripherals back to controller
    for (int i = 0; i < 2; i++) {
        enum peripheral_type peripheral_type =
            (enum peripheral_type)(*peripheral_types_settings_map[port][i]);

        // If peripheralType is out of range, change the settings for this
        // controller and peripheral port to default
        if (peripheral_type < PERIPHERAL_NONE ||
            peripheral_type >= PERIPHERAL_TYPE_COUNT) {
            xemu_save_peripheral_settings(port, i, PERIPHERAL_NONE, NULL);
            peripheral_type = PERIPHERAL_NONE;
        }

        const char *param = *peripheral_params_settings_map[port][i];

        if (peripheral_type == PERIPHERAL_XMU) {
            if (param != NULL && strlen(param) > 0) {
                // This is an XMU and needs to be bound to this controller
                if (qemu_access(param, R_OK | W_OK) == 0) {
                    bound_controllers[port]->peripheral_types[i] =
                        peripheral_type;
                    bound_controllers[port]->peripherals[i] =
                        g_malloc(sizeof(XmuState));
                    memset(bound_controllers[port]->peripherals[i], 0,
                           sizeof(XmuState));
                    bool did_bind = xemu_input_bind_xmu(port, i, param, true);
                    if (did_bind) {
                        char *buf =
                            g_strdup_printf("Connected XMU %s to port %d%c",
                                            param, port + 1, 'A' + i);
                        xemu_queue_notification(buf);
                        g_free(buf);
                    }
                } else {
                    char *buf =
                        g_strdup_printf("Unable to bind XMU at %s to port %d%c",
                                        param, port + 1, 'A' + i);
                    xemu_queue_error_message(buf);
                    g_free(buf);
                }
            }
        }
    }
}

void xemu_input_bind_passthrough(int index, LibusbDevice *state, int save)
{
    if(bound_libusb_devices[index]) {
        assert(bound_libusb_devices[index]->device != NULL);
        Error *err = NULL;
        qdev_unplug((DeviceState *)bound_libusb_devices[index]->device, &err);
        assert(err == NULL);

        bound_libusb_devices[index]->bound = -1;
        bound_libusb_devices[index]->device = NULL;
        bound_libusb_devices[index] = NULL;
    }

    if(save) {
        char guid_buf[35] = { 0 };
        if(state) {
            // format:     hex       hex        int      string
            //         USB\vendor_id:product_id:host_bus:host_port
            snprintf(guid_buf, sizeof(guid_buf), "USB\\%04x:%04x:%d:%s", state->vendor_id, state->product_id, state->host_bus, state->host_port);
        }
        xemu_settings_set_string(port_index_to_settings_key_map[index], guid_buf);
        xemu_settings_set_string(port_index_to_driver_settings_key_map[index], bound_drivers[index]);
    }

    if(strcmp(bound_drivers[index], DRIVER_USB_PASSTHROUGH) == 0) {
        if(state) {
            if (state->bound >= 0) {
                // Device was already bound to another port. Unbind it.
                xemu_input_bind_passthrough(state->bound, NULL, state->bound != index);
            }

            bound_libusb_devices[index] = state;
            bound_libusb_devices[index]->bound = index;

            const int port_map[4] = {3, 4, 1, 2};

            if(state->internal_hub_ports > 0) {
                // Create controller's internal USB hub.
                char *port = g_strdup_printf("1.%d", port_map[index]);
                DeviceState *usbhub_dev = xemu_bind_usb_hub(state->internal_hub_ports, port);
                g_free(port);

                // Create XID controller. This is connected to Port 1 of the controller's internal USB Hub
                port = g_strdup_printf("1.%d.1", port_map[index]);
                DeviceState *controller_dev = xemu_bind_usb_host(state->host_bus, state->host_port, port);
                g_free(port);

                if(state->internal_hub_ports > 1) {
                    for(int i = 1; i < state->internal_hub_ports; i++) {
                        port = g_strdup_printf("1.%d.%d", port_map[index], i+1);
                        char *hostport = g_strdup_printf("%.*s%d", (int)strlen(state->host_port) - 1, state->host_port, i+1);
                        DeviceState *expansion_port_dev = xemu_bind_usb_host(state->host_bus, hostport, port);
                        g_free(port);
                        g_free(hostport);
                        object_unref(OBJECT(expansion_port_dev));
                    }
                }

                // Unref for eventual cleanup
                object_unref(OBJECT(usbhub_dev));
                object_unref(OBJECT(controller_dev));
                
                state->device = usbhub_dev;
            } else {
                // Create XID controller. This is connected to Port 1 of the controller's internal USB Hub
                char *port = g_strdup_printf("1.%d", port_map[index]);
                DeviceState *controller_dev = xemu_bind_usb_host(state->host_bus, state->host_port, port);
                g_free(port);

                // Unref for eventual cleanup
                object_unref(OBJECT(controller_dev));

                state->device = controller_dev;
            }
        }
    }
}

void xemu_input_set_test_mode(int enabled)
{
    test_mode = enabled;
}

int xemu_input_get_test_mode(void)
{
    return test_mode;
}

void xemu_input_reset_input_mapping(ControllerState *state)
{
    if (state->type == INPUT_DEVICE_SDL_GAMEPAD) {
        char guid[35] = { 0 };
        SDL_GUIDToString(state->sdl_joystick_guid, guid, sizeof(guid));
        xemu_settings_reset_controller_mapping(guid);
    } else if (state->type == INPUT_DEVICE_SDL_KEYBOARD) {
        xemu_settings_reset_keyboard_mapping();
    }
}
void xemu_input_reset_sbc_mapping(ControllerState *state)
{
    if (state->type == INPUT_DEVICE_SDL_GAMEPAD) {
        char guid[35] = { 0 };
        SDL_GUIDToString(state->sdl_joystick_guid, guid, sizeof(guid));
        xemu_settings_reset_controller_mapping(guid);
    } else if (state->type == INPUT_DEVICE_SDL_KEYBOARD) {
        xemu_settings_reset_sbc_keyboard_mapping();
    }
}