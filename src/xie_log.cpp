#include "xie_log.h"
#include <time.h>
#include <iostream>

int initLOGX()
{
#ifdef USE_SPDLOG
    try 
    {
        auto logger = spdlog::basic_logger_mt("cc", "logs/basic-log.txt");
        logger->set_level(spdlog::level::debug);
        logger->debug("Start>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cout << "Log init failed: " << ex.what() << std::endl;
    }
#endif
    
    return 0;
}



int destroyLOGX()
{
#ifdef USE_SPDLOG
    spdlog::get("cc")->debug("End<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
#endif
    return 0;
}



double TimeNow()
{
    struct timespec res;
    clock_gettime(CLOCK_REALTIME , &res);
    return   res.tv_sec + ((double)res.tv_nsec) / 1e9;
}