cmake_minimum_required(VERSION 3.20)
project(webserv)

SET(CMAKE_CXX_FLAGS "-Wall -Wextra -fsanitize=address")

set(CMAKE_CXX_STANDARD 98)

include_directories(autoindex cgi event http settingsManager utils)

add_executable(webserv
        main.cpp

        event/EventPool.h
        event/EventPool.cpp
        event/kqueue.cpp
        event/kqueue.h

        utils/TcpSocket.h
        utils/TcpSocket.cpp
        utils/Logger.cpp
        utils/utils.cpp
        utils/utils.h
        utils/CustomTime.h
        utils/CustomTime.cpp

        http/Request.h
        http/Request.cpp
        http/Response.h
        http/Response.cpp
        http/http.cpp
        http/http.h
        http/httpExceptions.h

        settingsManager/SettingsManager.hpp
        settingsManager/SettingsManager.cpp
        settingsManager/Server.hpp
        settingsManager/Server.cpp
        settingsManager/ConfigParser.hpp
        settingsManager/ConfigParser.cpp
        settingsManager/Route.hpp
        settingsManager/Route.cpp

        cgi/Cgi.hpp
        cgi/Cgi.cpp

        autoindex/Autoindex.hpp
        autoindex/Autoindex.cpp
        )
