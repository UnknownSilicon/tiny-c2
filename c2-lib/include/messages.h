#pragma once
#include <stdint.h>
#include "capabilities.h"

#define PROTO_VER 1

typedef enum TC2_MESSAGE_TYPE {
    UNKNOWN,
    INIT,
    ARRAY,
    ARRAY_STOP,
    CAPABILITY
} TC2_MESSAGE_TYPE_ENUM;

/*
FIXED messages represent a message with a fixed sized struct.
These can always be read directly into a struct.

DYNAMIC messages represent a message where a part of the message
does not have a constant size. This is useful for quickly and easily
transmitting a large amount of unpredictable length data.
Here, the dynamic portion of the message must be the last element of the message.
The server will read as much data into the struct as possible, then read the remaining
portion of the message into the relevant data.

DYNAMIC messages SHOULD be used sparingly due to their room for error.

DYNAMIC messages SHOULD NOT be used when there is a known number of fixed size structs.
In this case, send an ARRAY message, followed by the number of elements.
Then, send a ARRAY_STOP mesasge to finish it.
*/
typedef enum TC2_MESSAGE_STYLE {
    FIXED,
    DYNAMIC
} TC2_MESSAGE_STYLE_ENUM;

struct tc2_msg_init {
    uint8_t iv[16];
    uint8_t enc_id[32];
};
// TC2_MESSAGE_STYLE_ENUM TC2_MSG_INIT_STYLE = FIXED;

struct tc2_array {
    TC2_MESSAGE_TYPE_ENUM type;
    uint32_t num_elements;
};
// TC2_MESSAGE_STYLE_ENUM TC2_MSG_ARRAY_STYLE = FIXED;

struct tc2_array_stop {
};
// TC2_MESSAGE_STYLE_ENUM TC2_MSG_ARRAY_STOP_STYLE = FIXED;

struct tc2_capability {
    TC2_CAPABILITY_ENUM cap;
};
// TC2_MESSAGE_STYLE_ENUM TC2_MSG_CAPABILITY_STYLE = FIXED;

struct tc2_msg_preamble {
    TC2_MESSAGE_TYPE_ENUM type;
    uint32_t len;   
};