// To give your project a unique name, this code must be
// placed into a .c file (its own tab).  It can not be in
// a .cpp file or your main sketch (the .ino file).

#include "config.h"

#ifdef CWKEYERSHIELD
#undef USBMIDI
#endif

#if defined(CWKEYERSHIELD) || defined(USBMIDI)
#include "usb_names.h"
#endif

#ifdef CWKEYERSHIELD
// Name = "KeyerShield"
#define MIDI_NAME   {'K', 'e', 'y', 'e', 'r', 'S', 'h', 'i', 'e', 'l', 'd'}
#define MIDI_NAME_LEN  11
#endif

#ifdef USBMIDI
// Name = "Keyer"
#define MIDI_NAME   {'K', 'e', 'y', 'e', 'r'}
#define MIDI_NAME_LEN 5
#endif


#if defined(CWKEYERSHIELD) || defined(USBMIDI)
// Do not change this part.  This exact format is required by USB.
struct usb_string_descriptor_struct usb_string_product_name = {
        2 + MIDI_NAME_LEN * 2,
        3,
        MIDI_NAME
};
#endif
