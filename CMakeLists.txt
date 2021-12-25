cmake_minimum_required(VERSION 3.20)
project(webserv)

set(CMAKE_CXX_STANDARD 98)

add_executable(webserv
        main.cpp
        TcpSocket.h
        TcpSocket.cpp
        EventPool.h
        EventPool.cpp
        Logger.cpp
        utils.cpp
        Request.h
        Request.cpp
        Response.h
        Response.cpp
        kqueue.cpp
        http.cpp
        http.h

        settingsManager/SettingsManager.hpp
        settingsManager/SettingsManager.cpp
        settingsManager/Server.hpp
        settingsManager/Server.cpp
        )
