cmake_minimum_required(VERSION 3.31)
project(drogon_starter)

set(CMAKE_CXX_STANDARD 20)

add_executable(drogon_starter src/main.cpp
        src/core/Error.hpp
        src/core/Exception.hpp
        src/core/Response.hpp
        src/core/Constants.hpp
        src/utils/Redis.hpp
        src/utils/Redis.cpp
        src/utils/Crypto.hpp
        src/lock/UserLock.hpp
        src/lock/UserLock.cpp
        src/queue/MessageQueue.hpp
        src/queue/MessageQueue.cpp
        src/middleware/JwtFilter.hpp
        src/middleware/JwtFilter.cpp
        src/middleware/LogFilter.hpp
        src/middleware/LogFilter.cpp
        src/services/AuthService.hpp
        src/services/AuthService.cpp
        src/services/UserService.hpp
        src/services/UserService.cpp
        src/controllers/AuthController.hpp
        src/controllers/AuthController.cpp
        src/controllers/UserController.hpp
        src/controllers/UserController.cpp)

find_package(Drogon CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(hiredis_ssl CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Drogon::Drogon
        jwt-cpp::jwt-cpp
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        hiredis::hiredis
        hiredis::hiredis_ssl
)
