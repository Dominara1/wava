#include "shared.h"

EXP_FUNC wava_version wava_version_host_get(void) {
    wava_version v;
    v.major = WAVA_VERSION_MAJOR;
    v.minor = WAVA_VERSION_MINOR;
    v.tweak = WAVA_VERSION_TWEAK;
    v.patch = WAVA_VERSION_PATCH;
    return v;
}

EXP_FUNC bool wava_version_less(wava_version host, wava_version target)
{
    if(target.major > host.major) {
        return false;
    } else if (target.major < host.major) {
        return true;
    }

    if(target.minor > host.minor) {
        return false;
    } else if (target.minor < host.minor) {
        return true;
    }

    if(target.tweak > host.tweak) {
        return false;
    } else if (target.tweak < host.tweak) {
        return true;
    }

    if(target.patch > host.patch) {
        return false;
    } else if (target.patch < host.patch) {
        return true;
    }

    return false;
}

EXP_FUNC bool wava_version_greater(wava_version host, wava_version target)
{
    if(target.major > host.major) {
        return true;
    } else if (target.major < host.major) {
        return false;
    }

    if(target.minor > host.minor) {
        return true;
    } else if (target.minor < host.minor) {
        return false;
    }

    if(target.tweak > host.tweak) {
        return true;
    } else if (target.tweak < host.tweak) {
        return false;
    }

    if(target.patch > host.patch) {
        return true;
    } else if (target.patch < host.patch) {
        return false;
    }

    return false;
}

EXP_FUNC bool wava_version_equal(wava_version host, wava_version target)
{
    if(target.major == host.major && target.minor == host.minor &&
        target.tweak == host.tweak && target.patch == host.patch) {
        return true;
    }
    return false;
}

EXP_FUNC bool wava_version_breaking_check(wava_version target)
{
    // list of all versions that broke something in terms of internal variables
    wava_version breaking_versions[] = {
        {0, 7, 1, 1},
    };

    for(size_t i = 0; i < sizeof(breaking_versions)/sizeof(wava_version); i++) {
        if(wava_version_less(breaking_versions[i], target)) {
            return true;
        }
    }

    return false;
}

EXP_FUNC WAVA_VERSION_COMPATIBILITY wava_version_verify(wava_version target)
{
    wava_version host = wava_version_host_get();

    if(wava_version_equal(host, target)) {
        return WAVA_VERSIONS_COMPATIBLE;
    }

    if(wava_version_breaking_check(target)) {
        wavaError("The version of the module is NOT compatible with the current build");
        return WAVA_VERSIONS_INCOMPATIBLE;
    }

    if(wava_version_greater(host, target)) {
        wavaError("Please update WAVA to run this.");
        return WAVA_VERSIONS_INCOMPATIBLE;
    }

    wavaWarn("Versions may not be compatbile");
    return WAVA_VERSIONS_UNKNOWN;
}

