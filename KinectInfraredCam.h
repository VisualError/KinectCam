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
    /*static void StaticUnInit();*/

    KinectInfraredCam() = default;
    ~KinectInfraredCam() = default;
    KinectInfraredCam(const KinectInfraredCam&) = delete;
    KinectInfraredCam& operator=(const KinectInfraredCam&) = delete;
private:
    static INuiSensor* m_pNuiSensor;
    static HANDLE m_hNextVideoFrameEvent;
    static HANDLE m_pVideoStreamHandle;
    /*static long m_refCount;*/
};