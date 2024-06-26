#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "shared.h"

#ifdef __linux__
    #include <execinfo.h>
#endif

// internal stuff below

enum WAVA_MESSAGE_TYPE {
    WAVA_LOG_ERROR, WAVA_LOG_WARN, WAVA_LOG_NORM, WAVA_LOG_SPAM
};

static void __internal_wavaMsgHnd(enum WAVA_MESSAGE_TYPE mes, const char *fmt,
        const char *func, const char *file, int line, va_list list) {

    // please don't let this be a memory bug
    char newFmt[512];

    FILE *output = stdout;

    bool stack_trace = false;
    bool debug_details = false;
    bool ignore_spam = true;
    char *indicator = "[BUG!FIXTHISRIGHTTHEFUCKNOW]"; // ideally never shown

    if(getenv("WAVA_DEBUG"))
        debug_details = true;

    if(getenv("WAVA_TRACE"))
        stack_trace = true;

    if(getenv("WAVA_SPAMMY"))
        ignore_spam = false;

    if(getenv("WAVA_SCREAM")) {
        debug_details = true;
        stack_trace = true;
        ignore_spam = false;
    }

    bool should_trace = false;

    // process message headers
    switch(mes) {
        case WAVA_LOG_ERROR:
            output = stderr;
            should_trace = true;
            indicator = "[ERROR]";
            break;
        case WAVA_LOG_WARN:
            should_trace = true;
            indicator = "[WARN]";
            break;
        case WAVA_LOG_NORM:
            if(ignore_spam) return;
            indicator = "[INFO]";
            break;
        case WAVA_LOG_SPAM:
            if(ignore_spam) return;
            indicator = "[SPAM]";
            break;
    }

    if(debug_details)
        snprintf(newFmt, 511, "%s %s at %s:%d - %s\n", indicator, func, file, line, fmt);
    else
        snprintf(newFmt, 511, "%s\n", fmt);

    vfprintf(output, newFmt, list);

    // add stack traces for better debugging
    if(stack_trace && should_trace) {
    #ifdef __linux__
        void *stack_pointers[12];
        int stack_lenght = 12;
        char **stack_names;

        stack_lenght = backtrace(stack_pointers, stack_lenght);

        stack_names = backtrace_symbols(stack_pointers, stack_lenght);

        // skip the first two functions as they are part of the logging system itself
        for(int i=2; i<stack_lenght; i++) {
            printf("at %s\n", stack_names[i]);
        }
    #endif
    }

    #ifdef __WIN32__
    {
        bool dontbother = false;
        char *messageTypeString = NULL;
        UINT messageBoxParams = MB_OK;
        switch(mes) {
            case WAVA_LOG_ERROR:
                messageTypeString = "Error";
                messageBoxParams |= MB_ICONERROR;
                break;
            case WAVA_LOG_WARN:
                messageTypeString = "Warning";
                messageBoxParams |= MB_ICONWARNING;
                break;
            default:
                // other cases are not worthy of an pop-up message
                dontbother = true;
                break;
        }
        if(!dontbother) {
            char windowsMessage[4096];
            snprintf(newFmt, 4095, "%s\nBy %s at %s:%d", fmt, func, file, line);
            vsnprintf(windowsMessage, 4095, newFmt, list);

            // TODO: Change icon
            MessageBox(NULL, windowsMessage, messageTypeString, messageBoxParams);
        }
    }
    #endif
}


// global stuff below

EXP_FUNC void __internal_wavaSpam(const char *func, const char *file, int line, const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    __internal_wavaMsgHnd(WAVA_LOG_SPAM, fmt, func, file, line, list);
    va_end(list);
}

EXP_FUNC void __internal_wavaLog(const char *func, const char *file, int line, const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    __internal_wavaMsgHnd(WAVA_LOG_NORM, fmt, func, file, line, list);
    va_end(list);
}

EXP_FUNC void __internal_wavaWarn(const char *func, const char *file, int line, const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    __internal_wavaMsgHnd(WAVA_LOG_WARN, fmt, func, file, line, list);
    va_end(list);
}

EXP_FUNC void __internal_wavaError(const char *func, const char *file, int line, const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    __internal_wavaMsgHnd(WAVA_LOG_ERROR, fmt, func, file, line, list);
    va_end(list);
}

EXP_FUNC void __internal_wavaDie(void) {
    exit(EXIT_FAILURE);
}

