#pragma once

#include <chrono>
#include <string>
#ifdef CV_DEBUG
#include <opencv2/opencv.hpp>
#endif
#include "xie_log.h"

using namespace std::chrono;


namespace xwb
{




class TimeCost
{
public:
    TimeCost(const std::string& tag)
        :_tag(tag)
    {
        start_tp = high_resolution_clock::now();
    }

    ~TimeCost()
    {
        high_resolution_clock::time_point end_tp = high_resolution_clock::now();
        duration<double> time_span = duration_cast<duration<double>>(end_tp - start_tp);
        double cost = time_span.count();
        LOGD("[TIME] %.8f: %s", cost, _tag.c_str());
    }


private:
    high_resolution_clock::time_point start_tp;
    std::string _tag;
};


} // namespace xwb

#if 0
#define TIME_COST_FUNCTION xwb::TimeCost a(__FUNCTION__);
#define TIME_COST_FUNCTION_LINE xwb::TimeCost a(std::string(__FUNCTION__) + "_" + std::to_string(__LINE__));
#else
#define TIME_COST_FUNCTION 
#define TIME_COST_FUNCTION_LINE 
#endif




