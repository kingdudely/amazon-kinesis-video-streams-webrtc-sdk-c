#include "../../Include_i.h"

STATUS createPayloadForG711(UINT32 mtu, PBYTE frameData, UINT32 frameSize, PBYTE payloadBuffer, PUINT32 payloadLength, PUINT32 payloadSubLength,
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

STATUS depayG711FromRtpPayload(PBYTE payload, UINT32 payloadLength, PBYTE frameBuffer, PUINT32 frameLength, PBOOL isStart)
{
    UNUSED_PARAM(payload);
    UNUSED_PARAM(payloadLength);
    UNUSED_PARAM(frameBuffer);
    UNUSED_PARAM(frameLength);
    UNUSED_PARAM(isStart);
    return STATUS_NOT_IMPLEMENTED;
}
