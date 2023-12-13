#ifndef __JNI_GPU_COMPOSE_LOG_H__
#define __JNI_GPU_COMPOSE_LOG_H__

//#undef LOG_TAG
#define LOG_TAG "compose"

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, ##__VA_ARGS__);
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ##__VA_ARGS__);

#else

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

enum LOG_LEVEL {
    ANDROID_LOG_ERROR = 0,
    ANDROID_LOG_WARN,
    ANDROID_LOG_INFO,
    ANDROID_LOG_DEBUG,
};

static int init_log() {
    char *level = getenv("RK_DEMO_LOG_LEVEL");

    if (level == nullptr) {
        level = (char *)"0";
    }

    return atoi(level);
}

static int loglevel = init_log();

static size_t getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static size_t startTime = getCurrentTime();

#define LOG(level, _str, ...) \
        do {        \
            if (level <= loglevel) {    \
                size_t endTime = getCurrentTime();  \
                fprintf(stdout, "%ld " LOG_TAG " %s(%d): " _str , endTime-startTime, __FUNCTION__, __LINE__, ## __VA_ARGS__);    \
            }   \
        } while(0)

#define LOGD(_str, ...) LOG(ANDROID_LOG_DEBUG, _str , ## __VA_ARGS__)
#define LOGI(_str, ...) LOG(ANDROID_LOG_INFO, _str , ## __VA_ARGS__)
#define LOGW(_str, ...) LOG(ANDROID_LOG_WARN, _str , ## __VA_ARGS__)
#define LOGE(_str, ...) LOG(ANDROID_LOG_ERROR, _str , ## __VA_ARGS__)
#endif

#endif //__JNI_GPU_COMPOSE_LOG_H__