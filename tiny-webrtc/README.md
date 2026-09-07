# Tiny WebRTC Host

This directory is the new clean C++ implementation for the remote-desktop host.

The design is intentionally specialized instead of being a general WebRTC stack.

## Rules

The host allows one client at a time.
A new authenticated client replaces the old client.

There is no KVS signaling, TURN, SDP, JSON signaling, incoming audio decoding, incoming video decoding, or generic codec negotiation.

The signaling channel only authenticates the browser and exchanges binary ICE candidates.
IPv4 candidates are 6 bytes: 4-byte address followed by 2-byte network-order port.
IPv6 candidates are 18 bytes: 16-byte address followed by 2-byte network-order port.

The media path accepts already packetized RTP. H.264 and Opus encoders are separate replaceable blocks, and packetizers are separate replaceable blocks.

H.264 encoders can be supplied by Media Foundation, VideoToolbox, x264, or another implementation without changing the WebRTC core.

The core keeps RTP, RTCP, NACK, PLI, SRTP, DTLS, ICE, STUN, SCTP, and generic negotiated DataChannels separate.

DataChannel IDs are chosen by the application. The remote-desktop application uses fixed IDs, but the WebRTC core does not hardcode their meaning.

## Platform strategy

Windows uses Winsock, the Windows WebSocket Protocol Component API, and Schannel.
macOS uses Network.framework, NWProtocolWebSocket, and Network.framework DTLS.
Linux uses POSIX sockets and tiny in-tree WebSocket and DTLS implementations.

## Current status

The repository currently contains the architecture and interfaces only. The protocol implementations are deliberately being filled in one block at a time so each block can be tested and optimized independently.

The target is a small, fast, single-client remote-desktop WebRTC host rather than a general-purpose WebRTC library.
