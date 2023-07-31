#pragma once

#include <string>
#include <chrono>
#include <cstdio>

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
        printf("[TIME] %.8f: %s\n", cost, _tag.c_str());
    }


private:
    high_resolution_clock::time_point start_tp;
    std::string _tag;
};

void setDebugFilename(const std::string& filename);
std::string getDebugFilename();
std::string getSaveFilename(const std::string& ext);

} // namespace xwb


#define TIME_COST_FUNCTION xwb::TimeCost a(__FUNCTION__);
#define TIME_COST_FUNCTION_LINE xwb::TimeCost a(std::string(__FUNCTION__) + "_" + std::to_string(__LINE__));