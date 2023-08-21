#ifndef __WAVA_SHARED_LOG_H
#define __WAVA_SHARED_LOG_H

// static analyser, please shut the fuck up
#ifdef SOURCE_PATH_SIZE
    #define __FILENAME__ (__FILE__ + SOURCE_PATH_SIZE)
#else
    #define __FILENAME__ __FILE__
#endif

extern void __internal_wavaSpam (const char *func, const char *file, int line, const char *fmt, ...);
extern void __internal_wavaLog  (const char *func, const char *file, int line, const char *fmt, ...);
extern void __internal_wavaWarn (const char *func, const char *file, int line, const char *fmt, ...);
extern void __internal_wavaError(const char *func, const char *file, int line, const char *fmt, ...);
extern void __internal_wavaDie  (void);

#ifdef DEBUG
    #define wavaSpam(fmt, ...)  __internal_wavaSpam (__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__);
    #define wavaLog(fmt, ...)   __internal_wavaLog  (__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__);
#else
    #define wavaSpam(fmt, ...)  /** nothing lol **/
    #define wavaLog(fmt, ...)   /** nothing lol **/
#endif

#define wavaWarn(fmt, ...)  __internal_wavaWarn (__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__);
#define wavaError(fmt, ...) __internal_wavaError(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__);
#define wavaBail(fmt, ...) { \
    __internal_wavaError(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
    __internal_wavaDie(); \
}

#define wavaReturnSpam(return_val, fmt, ...) { \
    __internal_wavaSpam(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
    return return_val; \
}
#define wavaReturnLog(return_val, fmt, ...) { \
    __internal_wavaLog(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
    return return_val; \
}
#define wavaReturnWarn(return_val, fmt, ...) { \
    __internal_wavaWarn(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
    return return_val; \
}
#define wavaReturnError(return_val, fmt, ...) { \
    __internal_wavaError(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
    return return_val; \
}


#define wavaReturnSpamCondition(condition, return_val, fmt, ...) { \
    if(condition) { \
        __internal_wavaSpam(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
        return return_val; \
    } \
}
#define wavaReturnLogCondition(condition, return_val, fmt, ...) { \
    if(condition) { \
        __internal_wavaLog(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
        return return_val; \
    } \
}
#define wavaReturnWarnCondition(condition, return_val, fmt, ...) { \
    if(condition) { \
        __internal_wavaWarn(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
        return return_val; \
    } \
}
#define wavaReturnErrorCondition(condition, return_val, fmt, ...) { \
    if(condition) { \
        __internal_wavaError(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
        return return_val; \
    } \
}

#define wavaSpamCondition(condition, fmt, ...) \
    if((condition)) { __internal_wavaSpam(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); }
#define wavaLogCondition(condition, fmt, ...) \
    if((condition)) { __internal_wavaLog(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); }
#define wavaWarnCondition(condition, fmt, ...) \
    if((condition)) { __internal_wavaWarn(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); }
#define wavaErrorCondition(condition, fmt, ...) \
    if((condition)) { __internal_wavaError(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); }
#define wavaBailCondition(condition, fmt, ...) { \
    if((condition)) { \
        __internal_wavaError(__func__, __FILENAME__, __LINE__, fmt, ## __VA_ARGS__); \
        __internal_wavaDie(); \
    } \
}


#endif

