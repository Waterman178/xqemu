#define TYPE_USB_XID "usb-xid"
#define USB_XID(obj) OBJECT_CHECK(USBXIDState, (obj), TYPE_USB_XID)

#include "hw/usb.h"

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
    uint8_t report_id; //FIXME: is this correct?
    uint8_t length;
    uint16_t left_actuator_strength;
    uint16_t right_actuator_strength;
} QEMU_PACKED XIDGamepadOutputReport;

typedef struct USBXIDState {
    USBDevice dev;
    USBEndpoint *intr;
    const XIDDesc *xid_desc;
    QemuInputHandlerState *hs;
    bool in_dirty;
    XIDGamepadReport in_state;
    XIDGamepadReport in_state_capabilities;
    XIDGamepadOutputReport out_state;
    XIDGamepadOutputReport out_state_capabilities;
} USBXIDState;

typedef struct USBXIDClass {
  USBDeviceClass parent;
  void (*realize)(USBDevice *dev, Error **errp);
  void (*update_input)(USBXIDState *s);
  void (*update_output)(USBXIDState *s);
} USBXIDClass;

#define USB_XID_CLASS(klass) \
     OBJECT_CLASS_CHECK(USBXIDClass, (klass), TYPE_USB_XID)
#define USB_XID_GET_CLASS(obj) \
     OBJECT_GET_CLASS(USBXIDClass, (obj), TYPE_USB_XID)

#define GAMEPAD_A                0
#define GAMEPAD_B                1
#define GAMEPAD_X                2
#define GAMEPAD_Y                3
#define GAMEPAD_BLACK            4
#define GAMEPAD_WHITE            5
#define GAMEPAD_LEFT_TRIGGER     6
#define GAMEPAD_RIGHT_TRIGGER    7

#define GAMEPAD_DPAD_UP          8
#define GAMEPAD_DPAD_DOWN        9
#define GAMEPAD_DPAD_LEFT        10
#define GAMEPAD_DPAD_RIGHT       11
#define GAMEPAD_START            12
#define GAMEPAD_BACK             13
#define GAMEPAD_LEFT_THUMB       14
#define GAMEPAD_RIGHT_THUMB      15

#define GAMEPAD_LEFT_THUMB_UP    16
#define GAMEPAD_LEFT_THUMB_DOWN  17
#define GAMEPAD_LEFT_THUMB_LEFT  18
#define GAMEPAD_LEFT_THUMB_RIGHT 19

#define GAMEPAD_RIGHT_THUMB_UP    20
#define GAMEPAD_RIGHT_THUMB_DOWN  21
#define GAMEPAD_RIGHT_THUMB_LEFT  22
#define GAMEPAD_RIGHT_THUMB_RIGHT 23

#define BUTTON_MASK(button) (1 << ((button) - GAMEPAD_DPAD_UP))
