#ifndef __WAVA_SHARED_CONFIG_H
#define __WAVA_SHARED_CONFIG_H

#include "shared/util/types.h"

// config entry macro (cursed)
#define WAVA_CONFIG_OPTION(T, name) \
   T name; bool name##_is_set_from_file

// config line macro
#define WAVA_CONFIG_GET_STRING(config, section, key, default_value, return_value) \
    __internal_wavaConfigGetString(config, section, key, default_value, &return_value, &return_value##_is_set_from_file)
#define WAVA_CONFIG_GET_BOOL(config, section, key, default_value, return_value) \
    __internal_wavaConfigGetBool(config, section, key, default_value, &return_value, &return_value##_is_set_from_file)
#define WAVA_CONFIG_GET_I32(config, section, key, default_value, return_value) \
    __internal_wavaConfigGetI32(config, section, key, default_value, &return_value, &return_value##_is_set_from_file)
#define WAVA_CONFIG_GET_U32(config, section, key, default_value, return_value) \
    __internal_wavaConfigGetU32(config, section, key, default_value, &return_value, &return_value##_is_set_from_file)
#define WAVA_CONFIG_GET_F64(config, section, key, default_value, return_value) \
    __internal_wavaConfigGetF64(config, section, key, default_value, &return_value, &return_value##_is_set_from_file)

typedef struct wava_config* wava_config_source;

extern void __internal_wavaConfigGetString(
        wava_config_source config,
        const char* section,
        const char* key,
        const char* default_value,
        char **value,
        bool *value_is_set);

extern void __internal_wavaConfigGetBool(
        wava_config_source config,
        const char* section,
        const char* key,
        bool default_value,
        bool *value,
        bool *value_is_set);

extern void __internal_wavaConfigGetI32(
        wava_config_source config,
        const char* section,
        const char* key,
        i32 default_value,
        i32 *value,
        bool *value_is_set);

extern void __internal_wavaConfigGetU32(
        wava_config_source config,
        const char* section,
        const char* key,
        u32 default_value,
        u32 *value,
        bool *value_is_set);

extern void __internal_wavaConfigGetF64(
        wava_config_source config,
        const char* section,
        const char* key,
        f64 default_value,
        f64 *value,
        bool *value_is_set);


extern bool  wavaConfigGetBool  (wava_config_source config, const char *section, const char *key, bool default_value);
extern i32   wavaConfigGetI32   (wava_config_source config, const char *section, const char *key, i32 default_value);
extern f64   wavaConfigGetF64   (wava_config_source config, const char *section, const char *key, f64 default_value);
extern char* wavaConfigGetString(wava_config_source config, const char *section, const char *key, const char* default_value);

extern wava_config_source wavaConfigOpen (const char *filename);
extern void               wavaConfigClose(wava_config_source config);

extern int    wavaConfigGetKeyNumber(wava_config_source config, const char *section);
extern char** wavaConfigGetKeys     (wava_config_source config, const char *section);
#endif

