//
// Created by Lajuana Bespin on 1/16/22.
//

//#include <ctime>
#include "CustomTime.h"

Time::customTime Time::now() {
    struct timeval tv = {};
    struct timezone tz = {};

    gettimeofday(&tv, &tz);

    return static_cast<customTime>(tv.tv_sec);
}
