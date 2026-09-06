/*******************************************
Main internal include file
*******************************************/
#ifndef __KINESIS_VIDEO_WEBRTC_CLIENT_INCLUDE_I__
#define __KINESIS_VIDEO_WEBRTC_CLIENT_INCLUDE_I__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#endif

////////////////////////////////////////////////////
// Project include files
////////////////////////////////////////////////////
#include <com/amazonaws/kinesis/video/webrtcclient/Include.h>

#ifdef KVS_USE_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#elif KVS_USE_MBEDTLS
#ifdef __has_include
#if __has_include(<mbedtls/build_info.h>)
#include <mbedtls/build_info.h>
#else
#include <mbedtls/version.h>
#endif
#else
#include <mbedtls/version.h>
#endif
#if defined(MBEDTLS_VERSION_NUMBER) && MBEDTLS_VERSION_MAJOR >= 4
#define MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS
#endif
#include <mbedtls/ssl.h>
#include <mbedtls/error.h>
#if defined(MBEDTLS_VERSION_NUMBER) && MBEDTLS_VERSION_MAJOR >= 4
#include <mbedtls/private/entropy.h>
#include <mbedtls/private/ctr_drbg.h>
#include <mbedtls/private/sha256.h>
#include <mbedtls/private/md5.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/private/rsa.h>
#include <mbedtls/private/ecp.h>
#include <mbedtls/private/bignum.h>
#else
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#if MBEDTLS_VERSION_NUMBER < 0x03000000
#include <mbedtls/certs.h>
#endif
#include <mbedtls/sha256.h>
#include <mbedtls/md5.h>
#endif
#endif

#ifdef USE_LIBSRTP3
#include <srtp3/srtp.h>
#else
#include <srtp2/srtp.h>
#endif

// INET/INET6 MUST be defined before usrsctp
#define INET  1
#define INET6 1
#include <usrsctp.h>

#if !defined __WINDOWS_BUILD__
#include <signal.h>
#include <sys/types.h>
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif
#include <sys/socket.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#endif

#define ICE_MAX_UFRAG_LEN 256
#define ICE_MAX_UPWD_LEN  256
#define STUN_MAX_USERNAME_LEN (UINT16) 512
#define STUN_MAX_REALM_LEN (UINT16) 128
#define STUN_MAX_NONCE_LEN (UINT16) 128
#define STUN_MAX_ERROR_PHRASE_LEN (UINT16) 128
#define IPV6_ADDRESS_LENGTH (UINT16) 16
#define IPV4_ADDRESS_LENGTH (UINT16) 4
#define CERTIFICATE_FINGERPRINT_LENGTH 160
#define MAX_UDP_PACKET_SIZE 65507

typedef enum {
    KVS_IP_FAMILY_TYPE_NOT_SET = (UINT16) 0x0000,
    KVS_IP_FAMILY_TYPE_IPV4 = (UINT16) 0x0001,
    KVS_IP_FAMILY_TYPE_IPV6 = (UINT16) 0x0002,
} KVS_IP_FAMILY_TYPE;

typedef struct {
    UINT16 family;
    UINT16 port;
    BYTE address[IPV6_ADDRESS_LENGTH];
    BOOL isPointToPoint;
} KvsIpAddress, *PKvsIpAddress;

typedef struct {
    KvsIpAddress ipv4Address;
    KvsIpAddress ipv6Address;
} DualKvsIpAddresses, *PDualKvsIpAddresses;

static inline BOOL IS_IPV4_ADDR(const PKvsIpAddress pAddress)
{
    return pAddress != NULL && pAddress->family == KVS_IP_FAMILY_TYPE_IPV4;
}

static inline BOOL IS_IPV6_ADDR(const PKvsIpAddress pAddress)
{
    return pAddress != NULL && pAddress->family == KVS_IP_FAMILY_TYPE_IPV6;
}

#define ALIGN_UP_TO_MACHINE_WORD(x) ROUND_UP((x), SIZEOF(SIZE_T))

typedef STATUS (*IceServerSetIpFunc)(UINT64, PCHAR, PDualKvsIpAddresses);
STATUS getIpAddrStr(PKvsIpAddress pKvsIpAddress, PCHAR pBuffer, UINT32 bufferLen);

////////////////////////////////////////////////////
// Project forward declarations
////////////////////////////////////////////////////
struct __SocketConnection;

////////////////////////////////////////////////////
// Project internal includes
////////////////////////////////////////////////////
#include "Threadpool/ThreadpoolContext.h"
#include "Crypto/IOBuffer.h"
#include "Crypto/Crypto.h"
#include "Crypto/Dtls.h"
#include "Crypto/Tls.h"
#include "Ice/Network.h"
#include "Ice/SocketConnection.h"
#include "Ice/ConnectionListener.h"
#include "Stun/Stun.h"
#include "Ice/IceUtils.h"
#include "Sdp/Sdp.h"
#include "Ice/IceAgent.h"
#include "Ice/IceAgentStateMachine.h"
#include "Srtp/SrtpSession.h"
#include "Sctp/Sctp.h"
#include "Rtp/RtpPacket.h"
#include "Rtcp/RtcpPacket.h"
#include "Rtcp/RollingBuffer.h"
#include "Rtcp/RtpRollingBuffer.h"
#include "PeerConnection/JitterBuffer.h"
#include "PeerConnection/PeerConnection.h"
#include "PeerConnection/Retransmitter.h"
#include "PeerConnection/SessionDescription.h"
#include "PeerConnection/Rtp.h"
#include "PeerConnection/Rtcp.h"
#include "PeerConnection/DataChannel.h"
#include "Rtp/Codecs/RtpH264Payloader.h"
#include "Rtp/Codecs/RtpOpusPayloader.h"
#include "Metrics/Metrics.h"

#define KVS_CONVERT_TIMESCALE(pts, from_timescale, to_timescale) (pts * to_timescale / from_timescale)

#ifdef __cplusplus
}
#endif
#endif /* __KINESIS_VIDEO_WEBRTC_CLIENT_INCLUDE_I__ */
