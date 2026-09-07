# Tiny WebRTC implementation status

This directory is the standalone WebRTC host. It is intentionally independent of the AWS KVS implementation.

Implemented in this pass:

- One-client host lifecycle.
- Binary signaling messages for 6-byte IPv4 and 18-byte IPv6 srflx candidates.
- Minimal RFC 6455 WebSocket framing and handshake interface.
- STUN Binding request/response primitives.
- Candidate storage and pair-selection primitives.
- RTP packet construction for already packetized media.
- RTCP NACK and PLI parsing.
- A bounded RTP retransmission cache.
- Generic negotiated DataChannel registration at the application layer.
- H.264 and Opus encoder/packetizer interfaces.

Not claimed complete yet:

- DTLS record/handshake implementation.
- SRTP cryptographic implementation.
- SCTP transport implementation.
- Full ICE connectivity checks and consent state machine.
- Native Windows/macOS DTLS integration.

Those pieces are protocol-critical and will be implemented against the actual browser wire behavior rather than using placeholder functions that could look complete but fail interoperability.
