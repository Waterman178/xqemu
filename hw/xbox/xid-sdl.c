/*
 * QEMU USB XID Devices
 *
 * Copyright (c) 2013 espes
 * Copyright (c) 2017 Jannik Vogel
 * Copyright (c) 2018 Matt Borgerson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "hw/hw.h"
#include "ui/console.h"
#include "hw/usb.h"
#include "hw/usb/desc.h"
#include "ui/input.h"

#include <SDL2/SDL.h>

//#define FORCE_FEEDBACK
#define UPDATE

/*
 * http://xbox-linux.cvs.sourceforge.net/viewvc/xbox-linux/kernel-2.6/drivers/usb/input/xpad.c
 * http://euc.jp/periphs/xbox-controller.en.html
 * http://euc.jp/periphs/xbox-pad-desc.txt
 */

#include "xid.h"

#define USB_CLASS_XID  0x58
#define USB_DT_XID     0x42

#define HID_GET_REPORT       0x01
#define HID_SET_REPORT       0x09
#define XID_GET_CAPABILITIES 0x01

#define TYPE_USB_XID_SDL "usb-xid-sdl" //FIXME: Rename to usb-xid-gamepad
#define USB_XID_SDL(obj) OBJECT_CHECK(USBXIDSDLState, (obj), TYPE_USB_XID_SDL)


typedef struct USBXIDSDLState {
    USBXIDState p;

    uint8_t device_index;
    SDL_GameController *sdl_gamepad;
#ifdef FORCE_FEEDBACK
    SDL_Haptic *sdl_haptic;
    int sdl_haptic_effect_id;
#endif
} USBXIDSDLState;

static void update_output(USBXIDSDLState *s) {
#ifdef FORCE_FEEDBACK
    if (s->sdl_haptic == NULL) { return; }

    USBXIDState* p = USB_XID(s)

    SDL_HapticLeftRight effect = {
        .type = SDL_HAPTIC_LEFTRIGHT,
        .length = SDL_HAPTIC_INFINITY,
        /* FIXME: Might be left/right inverted */
        .large_magnitude = p->out_state.right_actuator_strength,
        .small_magnitude = p->out_state.left_actuator_strength
    };

    if (s->sdl_haptic_effect_id == -1) {
        int effect_id = SDL_HapticNewEffect(s->sdl_haptic,
                                            (SDL_HapticEffect*)&effect);
        if (effect_id == -1) {
            fprintf(stderr, "Failed to upload haptic effect!\n");
            SDL_HapticClose(s->sdl_haptic);
            s->sdl_haptic = NULL;
            return;
        }
        SDL_HapticRunEffect(s->sdl_haptic, effect_id, 1);

        s->sdl_haptic_effect_id = effect_id;

    } else {
        SDL_HapticUpdateEffect(s->sdl_haptic, s->sdl_haptic_effect_id,
                               (SDL_HapticEffect*)&effect);
    }
#endif
}

static void update_input(USBXIDSDLState *s)
{

printf("SDL update\n");
    int i, state;

    USBXIDState* p = USB_XID(s);

#ifdef UPDATE
    SDL_GameControllerUpdate();
#endif

    /* Buttons */
    const int button_map_analog[6][2] = {
        { GAMEPAD_A, SDL_CONTROLLER_BUTTON_A },
        { GAMEPAD_B, SDL_CONTROLLER_BUTTON_B },
        { GAMEPAD_X, SDL_CONTROLLER_BUTTON_X },
        { GAMEPAD_Y, SDL_CONTROLLER_BUTTON_Y },
        { GAMEPAD_BLACK, SDL_CONTROLLER_BUTTON_LEFTSHOULDER },
        { GAMEPAD_WHITE, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER },
    };

    const int button_map_binary[8][2] = {
        { GAMEPAD_BACK, SDL_CONTROLLER_BUTTON_BACK },
        { GAMEPAD_START, SDL_CONTROLLER_BUTTON_START },
        { GAMEPAD_LEFT_THUMB, SDL_CONTROLLER_BUTTON_LEFTSTICK },
        { GAMEPAD_RIGHT_THUMB, SDL_CONTROLLER_BUTTON_RIGHTSTICK },
        { GAMEPAD_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_UP },
        { GAMEPAD_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_DOWN },
        { GAMEPAD_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_LEFT },
        { GAMEPAD_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT },
    };

#if 1
    for (i = 0; i < 6; i++) {
        state = SDL_GameControllerGetButton(s->sdl_gamepad,
                                            button_map_analog[i][1]);
        p->in_state.bAnalogButtons[button_map_analog[i][0]] = state ? 0xff : 0;
    }

    p->in_state.wButtons = 0;
    for (i = 0; i < 8; i++) {
        state = SDL_GameControllerGetButton(s->sdl_gamepad,
                                            button_map_binary[i][1]);
        if (state) {
            p->in_state.wButtons |= BUTTON_MASK(button_map_binary[i][0]);
        }
    }

    /* Triggers */
    state = SDL_GameControllerGetAxis(s->sdl_gamepad,
                                      SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    p->in_state.bAnalogButtons[GAMEPAD_LEFT_TRIGGER] = state >> 8;

    state = SDL_GameControllerGetAxis(s->sdl_gamepad,
                                      SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    p->in_state.bAnalogButtons[GAMEPAD_RIGHT_TRIGGER] = state >> 8;

    /* Analog sticks */
    p->in_state.sThumbLX = SDL_GameControllerGetAxis(s->sdl_gamepad,
                                SDL_CONTROLLER_AXIS_LEFTX);

    p->in_state.sThumbLY = -SDL_GameControllerGetAxis(s->sdl_gamepad,
                                SDL_CONTROLLER_AXIS_LEFTY) - 1;

    p->in_state.sThumbRX = SDL_GameControllerGetAxis(s->sdl_gamepad,
                                SDL_CONTROLLER_AXIS_RIGHTX);

    p->in_state.sThumbRY = -SDL_GameControllerGetAxis(s->sdl_gamepad,
                                SDL_CONTROLLER_AXIS_RIGHTY) - 1;
#endif
}

#if 0
static void usb_xid_handle_destroy(USBDevice *dev)
{
    USBXIDSDLState *s = DO_UPCAST(USBXIDSDLState, dev, dev);
    DPRINTF("xid handle_destroy\n");
#ifdef FORCE_FEEDBACK
    if (s->sdl_haptic) {
        SDL_HapticClose(s->sdl_haptic);
    }
#endif
    SDL_JoystickClose(s->sdl_gamepad);
}
#endif

static void usb_xbox_gamepad_unrealize(USBDevice *dev, Error **errp)
{
}


static void realize(USBDevice *dev, Error **errp)
{
    USBXIDSDLState *s = USB_XID_SDL(dev);

    printf("SDL Realize for index %d\n", s->device_index);

    /* FIXME: Make sure SDL was init before */
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)) {
        fprintf(stderr, "SDL failed to initialize joystick subsystem\n");
        exit(1);
    }

    s->sdl_gamepad = SDL_GameControllerOpen(s->device_index);
    if (s->sdl_gamepad == NULL) {
        /* FIXME: More appropriate qemu error handling */
        fprintf(stderr, "Couldn't open joystick %d\n", s->device_index);
        exit(1);
    }

    const char* name = SDL_GameControllerName(s->sdl_gamepad);
    printf("Found game controller %d (%s)\n", s->device_index, name);

#ifndef UPDATE
    /* We could update the joystick in the usb event handlers, but that would
     * probably pause emulation until data is ready + we'd also hammer SDL with
     * SDL_JoystickUpdate calls if the games are programmed poorly.
     */
    SDL_JoystickEventState(SDL_ENABLE);
#endif

#ifdef FORCE_FEEDBACK
    s->sdl_haptic = SDL_HapticOpenFromJoystick(s->sdl_gamepad);
    if (s->sdl_haptic == NULL) {
        fprintf(stderr, "Joystick doesn't support haptic\n");
    } else {
        if ((SDL_HapticQuery(s->sdl_haptic) & SDL_HAPTIC_LEFTRIGHT) == 0) {
            fprintf(stderr, "Joystick doesn't support necessary haptic effect\n");
            SDL_HapticClose(s->sdl_haptic);
            s->sdl_haptic = NULL;
        }
    }
#endif
}


static Property xid_sdl_properties[] = {
    DEFINE_PROP_UINT8("index", USBXIDSDLState, device_index, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void usb_xid_class_initfn(ObjectClass *klass, void *data)
{
    USBDeviceClass *uc = USB_DEVICE_CLASS(klass);
    USBXIDClass *p = USB_XID_CLASS(klass);

    p->realize = realize;
    p->update_input = update_input;
    p->update_output = update_output;

    //uc->handle_reset   = usb_xid_handle_reset;
    //uc->handle_control = usb_xid_handle_control;
    //uc->handle_data    = usb_xid_handle_data;
    // uc->handle_destroy = usb_xid_handle_destroy;
    //uc->handle_attach  = usb_desc_attach;

    printf("SDL init!\n");
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->props = xid_sdl_properties;
}


static const TypeInfo usb_xbox_gamepad_info = {
    .name          = TYPE_USB_XID_SDL,
    .parent        = TYPE_USB_XID,
    .instance_size = sizeof(USBXIDSDLState),
    .class_init    = usb_xid_class_initfn,
};

static void usb_xid_register_types(void)
{
    type_register_static(&usb_xbox_gamepad_info);
}

type_init(usb_xid_register_types)
