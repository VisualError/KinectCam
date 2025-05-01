#include <Shlobj.h>
#include "util.h"
#include "KinectInfraredCam.h"

bool g_flipImage = false;

//KinectCam::KinectCam() : 
//    m_hNextVideoFrameEvent(INVALID_HANDLE_VALUE),
//    m_pVideoStreamHandle(INVALID_HANDLE_VALUE),
//    m_pNuiSensor(NULL)
//{}

//KinectCam::~KinectCam() {
//    KinectCam::Nui_UnInit();
//}


//INuiSensor* KinectInfraredCam::m_pNuiSensor = nullptr;
//HANDLE KinectInfraredCam::m_hNextVideoFrameEvent = INVALID_HANDLE_VALUE;
//HANDLE KinectInfraredCam::m_pVideoStreamHandle = INVALID_HANDLE_VALUE;
//long KinectInfraredCam::m_refCount = 0;

HRESULT KinectInfraredCam::CreateFirstConnected() {
    INuiSensor* pNuiSensor;
    HRESULT hr;

    int iSensorCount = 0;
    hr = NuiGetSensorCount(&iSensorCount);
    if (FAILED(hr))
    {
        return hr;
    }

    // Look at each Kinect sensor
    for (int i = 0; i < iSensorCount; ++i)
    {
        // Create the sensor so we can check status, if we can't create it, move on to the next
        hr = NuiCreateSensorByIndex(i, &pNuiSensor);
        if (FAILED(hr))
        {
            continue;
        }

        // Get the status of the sensor, and if connected, then we can initialize it
        hr = pNuiSensor->NuiStatus();
        if (S_OK == hr)
        {
            m_pNuiSensor = pNuiSensor;
            break;
        }

        // This sensor wasn't OK, so release it since we're not using it
        pNuiSensor->Release();
    }

    if (NULL != m_pNuiSensor)
    {
        // Initialize the Kinect and specify that we'll be using color
        hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR);
        if (SUCCEEDED(hr))
        {
            // Create an event that will be signaled when color data is available
            m_hNextVideoFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            m_pNuiSensor->NuiCameraElevationSetAngle(20);
            // Open a color image stream to receive color frames
            hr = m_pNuiSensor->NuiImageStreamOpen(
                NUI_IMAGE_TYPE_COLOR_INFRARED,
                NUI_IMAGE_RESOLUTION_640x480,
                0,
                2,
                m_hNextVideoFrameEvent,
                &m_pVideoStreamHandle);
        }
    }

    if (NULL == m_pNuiSensor || FAILED(hr))
    {
        return E_FAIL;
    }

    return hr;
}

// TODO: fix bug where alt + f4 doesnt close camera
// TODO: fix bug where setting elevation to 0 somehow fixes and causes a bug? (bug: ir camera doesnt get shut off, but it prevents weird shit from happening)
void KinectInfraredCam::Nui_UnInit()
{
    if (m_pNuiSensor)
    {
        m_pNuiSensor->NuiCameraElevationSetAngle(0);
        m_pNuiSensor->NuiShutdown();
    }

    if (m_hNextVideoFrameEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hNextVideoFrameEvent);
        m_hNextVideoFrameEvent = INVALID_HANDLE_VALUE;
    }
     SafeRelease(m_pNuiSensor);
}


void KinectInfraredCam::Nui_GetCamFrame(BYTE* frameBuffer, int frameSize)
{
    HRESULT hr;
    NUI_IMAGE_FRAME imageFrame;

    if (WAIT_OBJECT_0 != WaitForSingleObject(m_hNextVideoFrameEvent, 100))
        return;

    hr = m_pNuiSensor->NuiImageStreamGetNextFrame(
        m_pVideoStreamHandle,
        0,
        &imageFrame);
    if (FAILED(hr))
    {
        return;
    }

    INuiFrameTexture* pTexture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect(0, &LockedRect, NULL, 0);
    if (LockedRect.Pitch != 0)
    {
        BYTE* pBuffer = (BYTE*)LockedRect.pBits;
        memcpy(frameBuffer, pBuffer, frameSize);
    }
    pTexture->UnlockRect(0);
    m_pNuiSensor->NuiImageStreamReleaseFrame(m_pVideoStreamHandle, &imageFrame);
}
