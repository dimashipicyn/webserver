cmake_minimum_required(VERSION 3.20)
project(webserv)

set(CMAKE_CXX_STANDARD 98)

add_executable(webserv
#        main.cpp
#        TcpSocket.h
#        TcpSocket.cpp
#        EventPool.h
#        EventPool.cpp
#        Logger.cpp
        utils.cpp
        utils.h
#        Request.h
#        Request.cpp
#        Response.h
#        Response.cpp
#        kqueue.cpp
#        http.cpp
#        http.h

        settingsManager/main.cpp
        settingsManager/SettingsManager.hpp
        settingsManager/SettingsManager.cpp
        settingsManager/Server.hpp
        settingsManager/Server.cpp
        settingsManager/ConfigParser.hpp
        settingsManager/ConfigParser.cpp
        settingsManager/Route.hpp
        settingsManager/Route.cpp
        )
