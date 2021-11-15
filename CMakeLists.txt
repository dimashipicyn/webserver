cmake_minimum_required(VERSION 3.20)
project(webserv)

set(CMAKE_CXX_STANDARD 98)

add_executable(webserv
        main.cpp
        Socket.cpp
        Socket.h
        EventPool.cpp
        EventPool.h
        Logger.cpp
        Logger.h
        Job.h
        utils.cpp
        Request.cpp
        Response.cpp)
