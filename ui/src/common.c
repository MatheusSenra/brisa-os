#include <strings.h>
#include <time.h>

#include "common.h"

static const char *const names[IN_COUNT] = {
    [IN_NONE] = "-",
    [IN_UP] = "UP", [IN_DOWN] = "DOWN", [IN_LEFT] = "LEFT", [IN_RIGHT] = "RIGHT",
    [IN_A] = "A", [IN_B] = "B", [IN_X] = "X", [IN_Y] = "Y",
    [IN_L1] = "L1", [IN_R1] = "R1", [IN_L2] = "L2", [IN_R2] = "R2",
    [IN_START] = "START", [IN_SELECT] = "SELECT", [IN_MENU] = "MENU",
    [IN_POWER] = "POWER", [IN_VOLUP] = "VOLUP", [IN_VOLDOWN] = "VOLDOWN",
};

const char *in_name(int btn)
{
    return (btn > 0 && btn < IN_COUNT) ? names[btn] : names[IN_NONE];
}

int in_from_name(const char *name)
{
    for (int i = 1; i < IN_COUNT; i++)
        if (strcasecmp(name, names[i]) == 0)
            return i;
    return IN_NONE;
}

uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}
