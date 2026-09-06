#define LOG_CLASS "MinimalPublisher"

#include "../Include_i.h"

#undef onInboundPacket
#undef addTransceiver

STATUS createJitterBuffer(FrameReadyFunc onFrameReadyFunc, FrameDroppedFunc onFrameDroppedFunc, DepayRtpPayloadFunc depayRtpPayloadFunc,
                          UINT32 maxLatency, UINT32 clockRate, UINT64 customData, PJitterBuffer* ppJitterBuffer)
{
    UNUSED_PARAM(onFrameReadyFunc);
    UNUSED_PARAM(onFrameDroppedFunc);
    UNUSED_PARAM(depayRtpPayloadFunc);
    UNUSED_PARAM(maxLatency);
    UNUSED_PARAM(customData);

    CHK(ppJitterBuffer != NULL && clockRate != 0, STATUS_NULL_ARG);
    *ppJitterBuffer = (PJitterBuffer) MEMCALLOC(1, SIZEOF(JitterBuffer));
    CHK(*ppJitterBuffer != NULL, STATUS_NOT_ENOUGH_MEMORY);
    (*ppJitterBuffer)->clockRate = clockRate;

CleanUp:
    return STATUS_SUCCESS;
}

STATUS freeJitterBuffer(PJitterBuffer* ppJitterBuffer)
{
    if (ppJitterBuffer != NULL) {
        SAFE_MEMFREE(*ppJitterBuffer);
    }
    return STATUS_SUCCESS;
}

VOID onInboundPacket(UINT64 customData, PBYTE buff, UINT32 buffLen)
{
    PKvsPeerConnection pc = (PKvsPeerConnection) customData;
    INT32 signedBuffLen = (INT32) buffLen;
    BOOL dtlsConnected = FALSE;

    if (pc == NULL || buff == NULL || signedBuffLen <= 2) {
        return;
    }

    if (buff[0] > 19 && buff[0] < 64) {
        if (STATUS_FAILED(dtlsSessionProcessPacket(pc->pDtlsSession, buff, &signedBuffLen))) {
            return;
        }

        if (STATUS_SUCCEEDED(dtlsSessionIsInitFinished(pc->pDtlsSession, &dtlsConnected)) && dtlsConnected) {
            if (pc->pSrtpSession == NULL) {
                if (STATUS_FAILED(allocateSrtp(pc))) {
                    return;
                }
            }

#ifdef ENABLE_DATA_CHANNEL
            if (ATOMIC_LOAD_BOOL(&pc->sctpIsEnabled)) {
                if (pc->pSctpSession == NULL && STATUS_FAILED(allocateSctp(pc))) {
                    return;
                }
                if (signedBuffLen > 0 && pc->pSctpSession != NULL) {
                    putSctpPacket(pc->pSctpSession, buff, signedBuffLen);
                }
            }
#endif
            changePeerConnectionState(pc, RTC_PEER_CONNECTION_STATE_CONNECTED);
        }
        return;
    }

    if ((buff[0] > 127 && buff[0] < 192) && pc->pSrtpSession != NULL) {
        if (buff[1] >= 192 && buff[1] <= 223) {
            if (STATUS_SUCCEEDED(decryptSrtcpPacket(pc->pSrtpSession, buff, &signedBuffLen))) {
                onRtcpPacket(pc, buff, signedBuffLen);
            }
        }
        /* RTP media from the browser is intentionally ignored: this endpoint is publish-only. */
    }
}

STATUS addTransceiver(PRtcPeerConnection pPeerConnection, PRtcMediaStreamTrack pRtcMediaStreamTrack,
                      PRtcRtpTransceiverInit pRtcRtpTransceiverInit, PRtcRtpTransceiver* ppRtcRtpTransceiver)
{
    PKvsPeerConnection pc = (PKvsPeerConnection) pPeerConnection;
    PKvsRtpTransceiver transceiver = NULL;
    PJitterBuffer jitterBuffer = NULL;
    UINT32 clockRate;
    UINT32 ssrc = (UINT32) RAND();
    UINT32 rtxSsrc = (UINT32) RAND();

    UNUSED_PARAM(pRtcRtpTransceiverInit);

    CHK(pc != NULL && pRtcMediaStreamTrack != NULL && ppRtcRtpTransceiver != NULL, STATUS_NULL_ARG);
    CHK(pRtcMediaStreamTrack->codec == RTC_CODEC_H264_PROFILE_42E01F_LEVEL_ASYMMETRY_ALLOWED_PACKETIZATION_MODE ||
            pRtcMediaStreamTrack->codec == RTC_CODEC_OPUS,
        STATUS_NOT_IMPLEMENTED);

    clockRate = pRtcMediaStreamTrack->codec == RTC_CODEC_OPUS ? OPUS_CLOCKRATE : VIDEO_CLOCKRATE;

    CHK_STATUS(createJitterBuffer(NULL, NULL, NULL, 0, clockRate, 0, &jitterBuffer));
    CHK_STATUS(createKvsRtpTransceiver(RTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY, pc, ssrc, rtxSsrc, pRtcMediaStreamTrack, jitterBuffer,
                                       pRtcMediaStreamTrack->codec, &transceiver));
    jitterBuffer = NULL;

    CHK_STATUS(doubleListInsertItemHead(pc->pTransceivers, (UINT64) transceiver));
    *ppRtcRtpTransceiver = (PRtcRtpTransceiver) transceiver;

    CHK_STATUS(timerQueueAddTimer(pc->timerQueueHandle, RTCP_FIRST_REPORT_DELAY, TIMER_QUEUE_SINGLE_INVOCATION_PERIOD,
                                  rtcpReportsCallback, (UINT64) transceiver, &transceiver->rtcpReportsTimerId));
    transceiver = NULL;

CleanUp:
    if (jitterBuffer != NULL) {
        freeJitterBuffer(&jitterBuffer);
    }
    if (transceiver != NULL) {
        freeKvsRtpTransceiver(&transceiver);
    }
    return STATUS_SUCCESS;
}
