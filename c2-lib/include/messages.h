#include <stdint.h>

typedef enum TC2_MESSAGE_TYPE {
    INIT,
} TC2_MESSAGE_TYPE_ENUM;

typedef enum TC2_MESSAGE_STYLE {
    FIXED,
    DYNAMIC
} TC2_MESSAGE_STYLE_ENUM;

struct tc2_msg_init {
    char key[64];
};
TC2_MESSAGE_TYPE_ENUM INIT_STYLE = FIXED;

struct tc2_msg_preamble {
    TC2_MESSAGE_TYPE_ENUM type;
    uint32_t len;   
};