#pragma once


#ifdef USE_SPDLOG
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
extern std::shared_ptr<spdlog::logger> spdfilelog;
#endif


int initLOGX();
int destroyLOGX();


double TimeNow();

#ifdef ANDROID
#include <android/log.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "xwb",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_DEBUG, "xwb", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "xwb", __VA_ARGS__)
#else
     #ifdef USE_SPDLOG
        #define LOGE(...) do{char buf[256]; snprintf(buf,256,__VA_ARGS__); spdlog::get("cc")->error(buf);}while(0)
        #define LOGI(...) do{char buf[256]; snprintf(buf,256,__VA_ARGS__); spdlog::get("cc")->info(buf);}while(0)
        #define LOGD(...)  do{char buf[256]; snprintf(buf,256,__VA_ARGS__); spdlog::get("cc")->debug(buf);}while(0)
    #else
        #define LOGE(...)    fprintf(stderr, "[E %.3f] ", TimeNow());   fprintf(stderr, __VA_ARGS__);  printf("\n")
        #define LOGI(...)    fprintf(stdout, "[I %.3f] ", TimeNow());   fprintf(stdout, __VA_ARGS__);  printf("\n")
        #define LOGD(...)    fprintf(stdout, "[D %.3f] ", TimeNow());   fprintf(stdout, __VA_ARGS__);  printf("\n")
    #endif
#endif