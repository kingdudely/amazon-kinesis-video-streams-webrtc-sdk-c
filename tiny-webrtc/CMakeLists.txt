cmake_minimum_required(VERSION 3.20)
project(tiny_webrtc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(tiny_webrtc STATIC
    src/WebRtcHost.cpp
    src/Rtp.cpp
    src/Rtcp.cpp
    src/ice/Ice.cpp
    src/ice/Stun.cpp
    src/dtls/Dtls.cpp
    src/srtp/Srtp.cpp
    src/sctp/Sctp.cpp
    src/signaling/Signaling.cpp
    src/video/H264Packetizer.cpp
    src/audio/OpusPacketizer.cpp
)

target_include_directories(tiny_webrtc PUBLIC include)

target_compile_features(tiny_webrtc PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(tiny_webrtc PRIVATE /O2 /EHs-c- /GR-)
else()
    target_compile_options(tiny_webrtc PRIVATE -Os -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections)
endif()

if (APPLE)
    target_sources(tiny_webrtc PRIVATE
        src/platform/macos/NetworkWebSocket.mm
        src/platform/macos/NetworkDtls.mm
    )
    target_link_libraries(tiny_webrtc PRIVATE
        "-framework Network"
        "-framework Security"
    )
elseif (WIN32)
    target_sources(tiny_webrtc PRIVATE
        src/platform/windows/NativeWebSocket.cpp
        src/platform/windows/SchannelDtls.cpp
    )
    target_link_libraries(tiny_webrtc PRIVATE websocket secur32 ws2_32)
else()
    target_sources(tiny_webrtc PRIVATE
        src/platform/linux/TinyWebSocket.cpp
        src/platform/linux/TinyDtls.cpp
    )
endif()

if (WIN32)
    target_compile_definitions(tiny_webrtc PRIVATE TINYRTC_WINDOWS=1)
elseif (APPLE)
    target_compile_definitions(tiny_webrtc PRIVATE TINYRTC_MACOS=1)
else()
    target_compile_definitions(tiny_webrtc PRIVATE TINYRTC_LINUX=1)
endif()
