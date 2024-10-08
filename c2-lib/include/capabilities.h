#ifndef CAPABILITIES_H
#define CAPABILITIES_H

#define FOREACH_CAPABILITY(CAP) \
        CAP(CAP_NONE)   \
        CAP(CAP_SYSTEM) \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum TC2_CAPABILITY {
    FOREACH_CAPABILITY(GENERATE_ENUM)
} TC2_CAPABILITY_ENUM;

static const char* CAPABILITY_STRING[] = {
    FOREACH_CAPABILITY(GENERATE_STRING)
};

#endif