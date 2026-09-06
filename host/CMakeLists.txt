cmake_minimum_required(VERSION 3.20)
project(minimal_webrtc_host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
set(NO_TESTS ON CACHE BOOL "" FORCE)
set(NO_MEDIA OFF CACHE BOOL "" FORCE)
set(NO_WEBSOCKET ON CACHE BOOL "" FORCE)
set(USE_GNUTLS OFF CACHE BOOL "" FORCE)
set(USE_MBEDTLS OFF CACHE BOOL "" FORCE)
set(USE_OPENSSL ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    libdatachannel
    GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
    GIT_TAG v0.24.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(libdatachannel)

FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.4.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ixwebsocket)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(minimal-webrtc-host
    src/main.cpp
    src/input.cpp
    src/input.hpp
    src/rtp_source.cpp
    src/rtp_source.hpp
)

target_link_libraries(minimal-webrtc-host PRIVATE
    datachannel
    ixwebsocket
    nlohmann_json::nlohmann_json
)

target_include_directories(minimal-webrtc-host PRIVATE src)

if(WIN32)
    target_link_libraries(minimal-webrtc-host PRIVATE user32)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(minimal-webrtc-host PRIVATE pthread)
endif()
