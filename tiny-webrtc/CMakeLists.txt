cmake_minimum_required(VERSION 3.20)
project(tiny_webrtc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(tiny_webrtc STATIC
    src/CryptoTiny.cpp
    src/WebRtcHost.cpp
    src/Rtp.cpp
    src/Rtcp.cpp
    src/ice/Ice.cpp
    src/ice/Stun.cpp
    src/dtls/Dtls.cpp
    src/srtp/Srtp.cpp
    src/sctp/Sctp.cpp
    src/signaling/Signaling.cpp
    src/platform/Socket.cpp
    src/video/H264Packetizer.cpp
    src/audio/OpusPacketizer.cpp
)

target_include_directories(tiny_webrtc PUBLIC include)
target_compile_features(tiny_webrtc PUBLIC cxx_std_20)
target_compile_definitions(tiny_webrtc PRIVATE
    $<$<PLATFORM_ID:Linux>:TINYRTC_LINUX=1>
    $<$<PLATFORM_ID:Windows>:TINYRTC_WINDOWS=1>
    $<$<PLATFORM_ID:Darwin>:TINYRTC_MACOS=1>
)

if (MSVC)
    target_compile_options(tiny_webrtc PRIVATE /O2 /EHs-c- /GR-)
else()
    target_compile_options(tiny_webrtc PRIVATE -Os -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections)
    target_link_options(tiny_webrtc PRIVATE -Wl,--gc-sections)
endif()

if (WIN32)
    target_sources(tiny_webrtc PRIVATE
        src/platform/windows/NativeWebSocket.cpp
        src/platform/windows/SchannelDtls.cpp
    )
    target_link_libraries(tiny_webrtc PRIVATE ws2_32 websocket secur32)
elseif (APPLE)
    target_sources(tiny_webrtc PRIVATE
        src/platform/macos/NetworkWebSocket.mm
        src/platform/macos/NetworkDtls.mm
    )
    set_source_files_properties(
        src/platform/macos/NetworkWebSocket.mm
        src/platform/macos/NetworkDtls.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    target_link_libraries(tiny_webrtc PRIVATE
        "-framework Network"
        "-framework Security"
    )
elseif (UNIX)
    target_sources(tiny_webrtc PRIVATE
        src/crypto/LinuxKernelCrypto.cpp
        src/platform/linux/TinyWebSocket.cpp
        src/platform/linux/TinyDtls.cpp
    )
    target_include_directories(tiny_webrtc PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel-crypto/include)
    target_link_libraries(tiny_webrtc PRIVATE pthread)

    find_program(KERNEL_MAKE make)
    if (KERNEL_MAKE)
        add_custom_target(tinycrypto_module
            COMMAND ${KERNEL_MAKE} -C ${CMAKE_CURRENT_SOURCE_DIR}/kernel-crypto
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/kernel-crypto
            COMMENT "Building tinycrypto Linux kernel module")
    endif()
endif()
