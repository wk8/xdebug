#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "phuck_off_logger.h"
#include "phuck_off_sanity_check.h"

static uint32_t phuck_off_sanity_check_state = 0;
static int phuck_off_sanity_check_sampling = PHUCK_OFF_DEFAULT_SANITY_CHECK_SAMPLING;

static int phuck_off_sanity_check_parse_sampling(const char* value, int* sampling_out) {
    char* end = NULL;
    long parsed;

    errno = 0;
    parsed = strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < 0 || parsed > 100) {
        return 0;
    }

    *sampling_out = (int) parsed;
    return 1;
}

void phuck_off_sanity_check_init(void) {
    const char* sampling_env;
    int sampling = PHUCK_OFF_DEFAULT_SANITY_CHECK_SAMPLING;

    sampling_env = getenv(PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR);
    if (sampling_env && !phuck_off_sanity_check_parse_sampling(sampling_env, &sampling)) {
        phuck_off_log(
            PHUCK_OFF_LOG_LEVEL_WARN,
            "Invalid %s=\"%s\", defaulting to %d",
            PHUCK_OFF_SANITY_CHECK_SAMPLING_ENV_VAR,
            sampling_env,
            PHUCK_OFF_DEFAULT_SANITY_CHECK_SAMPLING
        );
        sampling = PHUCK_OFF_DEFAULT_SANITY_CHECK_SAMPLING;
    }

    phuck_off_sanity_check_sampling = sampling;
    phuck_off_sanity_check_state = 0;

    if (sampling > 0 && sampling < 100) {
        phuck_off_sanity_check_state = ((uint32_t) time(NULL)) ^ (((uint32_t) getpid()) << 16);
        if (phuck_off_sanity_check_state == 0) {
            phuck_off_sanity_check_state = 0x9e3779b9u;
        }
    }
}

int phuck_off_sanity_check_should_sample(void) {
    uint32_t value;

    if (phuck_off_sanity_check_sampling <= 0) {
        return 0;
    }

    if (phuck_off_sanity_check_sampling >= 100) {
        return 1;
    }

    value = phuck_off_sanity_check_state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    if (value == 0) {
        value = 0x9e3779b9u;
    }
    phuck_off_sanity_check_state = value;

    return value <= (uint32_t) ((((uint64_t) phuck_off_sanity_check_sampling) * (uint64_t) UINT32_MAX) / 100u);
}
