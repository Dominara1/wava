#ifndef __WAVA_SHARED_VERSION_H
#define __WAVA_SHARED_VERSION_H
#include <stdbool.h>

typedef enum WAVA_VERSION_COMPATIBILITY {
    WAVA_VERSIONS_COMPATIBLE,
    WAVA_VERSIONS_INCOMPATIBLE,
    WAVA_VERSIONS_UNKNOWN,
} WAVA_VERSION_COMPATIBILITY;

typedef struct wava_version {
    int major;
    int minor;
    int tweak;
    int patch;
} wava_version;

wava_version               wava_version_host_get(void);
bool                       wava_version_less(wava_version host, wava_version target);
bool                       wava_version_greater(wava_version host, wava_version target);
bool                       wava_version_equal(wava_version host, wava_version target);
bool                       wava_version_breaking_check(wava_version target);
WAVA_VERSION_COMPATIBILITY wava_version_verify(wava_version target);

#define wava_version_get() (wava_version){WAVA_VERSION_MAJOR, WAVA_VERSION_MINOR, WAVA_VERSION_TWEAK, WAVA_VERSION_PATCH}

#endif

