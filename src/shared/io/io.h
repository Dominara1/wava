#include <stdint.h>
#include <stdbool.h>

// define directory breaks cuz windows sucks
#ifdef __WIN32__
    #define DIRBRK '\\'
#else
    #define DIRBRK '/'
#endif

#ifdef __linux__
    // I've gotten this from a stranger on the internet
    #include <linux/limits.h>
    #define MAX_PATH PATH_MAX
#endif

#ifdef __WIN32__
    #include <windows.h>
#endif

#ifdef __APPLE__
    #include <sys/syslimits.h>
    #define MAX_PATH PATH_MAX
#endif

#if defined(__unix__)||defined(__APPLE__)
    #define mkdir(dir) mkdir(dir, 0770)
#else
    #define mkdir(dir) mkdir(dir)
#endif

// WAVA file handlers
typedef enum WAVA_FILE_TYPE {
    WAVA_FILE_TYPE_CONFIG,           // where configuration files are stored
    WAVA_FILE_TYPE_OPTIONAL_CONFIG,  // where optional configuration files are stored
    WAVA_FILE_TYPE_CACHE,            // where cached files are stored (shader cache)
    WAVA_FILE_TYPE_PACKAGE,          // where files that are part of the packages are stored (assets or binaries)
    WAVA_FILE_TYPE_NONE,             // this is an error
    WAVA_FILE_TYPE_CUSTOM_READ,      // custom file that can only be readable (must include full path)
    WAVA_FILE_TYPE_CUSTOM_WRITE      // custom file that can be both readable and writable (must include full path)
} XF_TYPE;

// simulated data type
typedef struct data {
    size_t size;
    void*  data;
} RawData;

// WAVA event stuff
typedef enum WAVA_GRAHPICAL_EVENT {
    WAVA_REDRAW, WAVA_IGNORE, WAVA_RESIZE, WAVA_RELOAD,
    WAVA_QUIT
} XG_EVENT;

typedef struct WAVA_GRAHPICAL_EVENT_STACK {
    int pendingEvents;
    XG_EVENT *events;
} XG_EVENT_STACK;

// simulated event stack
extern void            pushWAVAEventStack    (XG_EVENT_STACK *stack, XG_EVENT event);
extern XG_EVENT        popWAVAEventStack     (XG_EVENT_STACK *stack);
extern XG_EVENT_STACK *newWAVAEventStack     ();
extern void            destroyWAVAEventStack (XG_EVENT_STACK *stack);
extern bool            pendingWAVAEventStack (XG_EVENT_STACK *stack);
extern bool            isEventPendingWAVA    (XG_EVENT_STACK *stack, XG_EVENT event);

// OS abstractions
extern           int wavaMkdir(const char *dir);
extern          bool wavaFindAndCheckFile(XF_TYPE type, const char *filename, char **actualPath);
extern unsigned long wavaSleep(unsigned long oldTime, int framerate);
extern unsigned long wavaGetTime(void);

// file/memory abstractions
extern RawData *wavaReadFile(const char *file);
extern void     wavaCloseFile(RawData *file);
extern void    *wavaDuplicateMemory(void *memory, size_t size);

