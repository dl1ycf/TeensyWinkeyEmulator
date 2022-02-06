// To give your project a unique name, this code must be
// placed into a .c file (its own tab).  It can not be in
// a .cpp file or your main sketch (the .ino file).

// Only works with Teensy3 and Teensy4.

#include "config.h"


#ifdef CWKEYERSHIELD

#include "usb_names.h"

// Name = "KeyerShield"
#define MIDI_NAME   {'K', 'e', 'y', 'e', 'r', 'S', 'h', 'i', 'e', 'l', 'd'}
#define MIDI_NAME_LEN  11

// Do not change this part.  This exact format is required by USB.
struct usb_string_descriptor_struct usb_string_product_name = {
        2 + MIDI_NAME_LEN * 2,
        3,
        MIDI_NAME
};
#endif
