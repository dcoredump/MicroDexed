#include "usb_names.h"
#define MIDI_NAME   {'M','i','c','r','o','D','e','x','e','d'}
#define MIDI_NAME_LEN 10

// Do not change this part.  This exact format is required by USB.

struct usb_string_descriptor_struct usb_string_product_name = {
        2 + MIDI_NAME_LEN * 2,
        3,
        MIDI_NAME
};
