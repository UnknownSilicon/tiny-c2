#pragma once
#include <stdint.h>
#include "capabilities.h"

#define PROTO_VER 1

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
In this case, send an MSG_ARRAY message, followed by the number of elements.
Then, send a MSG_ARRAY_STOP mesasge to finish it.
*/
typedef enum TC2_MESSAGE_STYLE {
    FIXED,
    DYNAMIC
} TC2_MESSAGE_STYLE_ENUM;


#define FOREACH_MESSAGE(MSG) \
        MSG(MSG_UNKNOWN, FIXED)    \
        MSG(MSG_INIT, FIXED)       \
        MSG(MSG_ARRAY, FIXED)      \
        MSG(MSG_ARRAY_STOP, FIXED) \
        MSG(MSG_CAPABILITY, FIXED) \
        MSG(MSG_SYSTEM, DYNAMIC)   \

#define GENERATE_MSG_ENUM(ENUM, TYPE) ENUM,
#define GENERATE_TYPES(ENUM, TYPE) TYPE,

typedef enum TC2_MESSAGE_TYPE {
    FOREACH_MESSAGE(GENERATE_MSG_ENUM)
} TC2_MESSAGE_TYPE_ENUM;

static const TC2_MESSAGE_STYLE_ENUM MESSAGE_TYPES[] = {
    FOREACH_MESSAGE(GENERATE_TYPES)
};


typedef enum TC2_ADD_REM {
    ADD,
    REMOVE
} TC2_ADD_REM_ENUM;

struct tc2_msg_init {
    uint8_t iv[16];
    uint8_t enc_id[32];
};

struct tc2_array {
    TC2_MESSAGE_TYPE_ENUM type;
    uint32_t num_elements;
};

struct tc2_array_stop {
};

struct tc2_capability {
    TC2_CAPABILITY_ENUM cap;
    TC2_ADD_REM_ENUM add_rem;
};

// Dynamic
struct tc2_system {

};

struct tc2_msg_preamble {
    TC2_MESSAGE_TYPE_ENUM type;
    uint32_t len;   
};