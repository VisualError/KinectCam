#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include "NuiApi.h"

class KinectInfraredCam
{
public:
    HRESULT CreateFirstConnected();
    void Nui_GetCamFrame(BYTE *frameBuffer, int frameSize);
    void Nui_UnInit();

    /*KinectInfraredCam() = default;
    ~KinectInfraredCam() = default;
    KinectInfraredCam(const KinectInfraredCam&) = delete;
    KinectInfraredCam& operator=(const KinectInfraredCam&) = delete;*/
private:
    INuiSensor* m_pNuiSensor;
    HANDLE m_hNextVideoFrameEvent;
    HANDLE m_pVideoStreamHandle;
};