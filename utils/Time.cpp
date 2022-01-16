//
// Created by Lajuana Bespin on 1/16/22.
//

#include <sys/time.h>
#include "Time.h"

Time::time Time::now() {
    struct timeval tv = {};
    struct timezone tz = {};

    gettimeofday(&tv, &tz);

    return static_cast<time>(tv.tv_sec);
}
