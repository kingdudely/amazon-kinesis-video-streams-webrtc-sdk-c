#include "../../Include_i.h"

STATUS createPayloadForH265(UINT32 mtu, PBYTE frameData, UINT32 frameSize, PBYTE payloadBuffer, PUINT32 payloadLength, PUINT32 payloadSubLength,
                            PUINT32 payloadSubLenSize)
{
    UNUSED_PARAM(mtu);
    UNUSED_PARAM(frameData);
    UNUSED_PARAM(frameSize);
    UNUSED_PARAM(payloadBuffer);
    UNUSED_PARAM(payloadLength);
    UNUSED_PARAM(payloadSubLength);
    UNUSED_PARAM(payloadSubLenSize);
    return STATUS_NOT_IMPLEMENTED;
}

STATUS getNextNaluLengthH265(PBYTE frameData, UINT32 frameSize, PUINT32 naluLength, PUINT32 startOffset)
{
    UNUSED_PARAM(frameData);
    UNUSED_PARAM(frameSize);
    UNUSED_PARAM(naluLength);
    UNUSED_PARAM(startOffset);
    return STATUS_NOT_IMPLEMENTED;
}

STATUS createPayloadFromNaluH265(UINT32 mtu, PBYTE nalu, UINT32 naluLength, PPayloadArray payloadArray, PUINT32 payloadLength, PUINT32 payloadSubLenSize)
{
    UNUSED_PARAM(mtu);
    UNUSED_PARAM(nalu);
    UNUSED_PARAM(naluLength);
    UNUSED_PARAM(payloadArray);
    UNUSED_PARAM(payloadLength);
    UNUSED_PARAM(payloadSubLenSize);
    return STATUS_NOT_IMPLEMENTED;
}

STATUS depayH265FromRtpPayload(PBYTE payload, UINT32 payloadLength, PBYTE frameBuffer, PUINT32 frameLength, PBOOL isStart)
{
    UNUSED_PARAM(payload);
    UNUSED_PARAM(payloadLength);
    UNUSED_PARAM(frameBuffer);
    UNUSED_PARAM(frameLength);
    UNUSED_PARAM(isStart);
    return STATUS_NOT_IMPLEMENTED;
}
