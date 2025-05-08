#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include "KinectVirtualCamera.h"

bool flipImage = true;
CUnknown* WINAPI CKinectVirtualSource::CreateInstance(LPUNKNOWN pUnk, HRESULT* phr) {
    ASSERT(phr);
    CUnknown* punk = new CKinectVirtualSource(pUnk, phr);
    return punk;
}

CKinectVirtualSource::CKinectVirtualSource(LPUNKNOWN pUnk, HRESULT* phr) : 
    CSource(NAME("Kinect Cam"), pUnk, CLSID_VirtualCam) 
{

    m_pBufferSize = 640 * 480 * 4;
    m_pBuffer = new BYTE[m_pBufferSize];
    m_kinected = false;

    m_paStreams = (CSourceStream**) new CKinectVirtualStream * [1];
    m_paStreams[0] = new CKinectVirtualStream(phr, this, L"Kinect Cam");
}

HRESULT CKinectVirtualSource::QueryInterface(REFIID riid, void** ppv)
{
    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
        return m_paStreams[0]->QueryInterface(riid, ppv);
    else
        return CSource::QueryInterface(riid, ppv);
}

CKinectVirtualSource::~CKinectVirtualSource()
{
    /*DbgLog((LOG_TRACE, 3, TEXT("CKinectVirtualSource::~CKinectVirtualSource")));
    printf("CKinectVirtualSource::~CKinectVirtualSource()\n");*/
    if (m_kinected) {
        m_kinectRGBCam.Nui_UnInit();
        m_kinected = false;
    }
    /*if (m_pBuffer) {
        delete[] m_pBuffer;
    }*/
    // Delete the output pin (Base class destructor might handle this, but explicit delete is safer)
    if (m_paStreams[0])
    {
        delete m_paStreams[0];
        delete[] m_paStreams;
    }
}

CKinectVirtualStream::CKinectVirtualStream(HRESULT* phr, CKinectVirtualSource* pParent, LPCWSTR pPinName)
    : CSourceStream(NAME("Kinect Cam"), phr, pParent, pPinName),
    m_pParent(pParent)
{
    GetMediaType(0,&m_mt);
    //DbgLog((LOG_TRACE, 3, TEXT("CKinectVirtualStream::CKinectVirtualStream")));
}

HRESULT CKinectVirtualStream::QueryInterface(REFIID riid, void** ppv)
{
    // Standard OLE stuff
    if (riid == _uuidof(IAMStreamConfig))
        *ppv = (IAMStreamConfig*)this;
    else if (riid == _uuidof(IKsPropertySet))
        *ppv = (IKsPropertySet*)this;
    else
        return CSourceStream::QueryInterface(riid, ppv);

    AddRef();
    return S_OK;
}


// Called when graph is run
HRESULT CKinectVirtualStream::OnThreadCreate()
{
    m_rtLastTime = 0;
    HRESULT hr = m_pParent->m_kinectRGBCam.CreateFirstConnected();
    m_pParent->m_kinected = SUCCEEDED(hr);
    return CSourceStream::OnThreadCreate();
} // OnThreadCreate

HRESULT CKinectVirtualStream::OnThreadDestroy()
{
    if (m_pParent->m_kinected) {
        m_pParent->m_kinectRGBCam.Nui_UnInit();
        m_pParent->m_kinected = false;
    }
    return CSourceStream::OnThreadDestroy();
} // OnThreadDestroy


//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CKinectVirtualStream::SetMediaType(const CMediaType* pmt)
{
    //DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
    HRESULT hr = CSourceStream::SetMediaType(pmt);
    return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CKinectVirtualStream::GetMediaType(int iPosition, CMediaType* pmt)
{
    if (iPosition < 0) return E_INVALIDARG;
    if (iPosition > 0) return VFW_S_NO_MORE_ITEMS;

    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount = 24;
    pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = 640;
    pvi->bmiHeader.biHeight = 480;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    pvi->AvgTimePerFrame = 10000000 / 30;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);
    pmt->SetSubtype(&MEDIASUBTYPE_RGB24);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
    *pmt = m_mt;
    // Work out the GUID for the subtype from the header info.
    /*const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);*/

    return NOERROR;

} // GetMediaType

HRESULT CKinectVirtualStream::CheckMediaType(const CMediaType* pMediaType)
{
    //DbgLog((LOG_TRACE, 3, TEXT("CKinectVirtualStream::CheckMediaType")));

    if (*pMediaType != m_mt)
        return E_INVALIDARG;
    return S_OK;

    //// Check if the major type is video
    //if (pMediaType->majortype != MEDIATYPE_Video)
    //{
    //    printf("CheckMediaType: Major type is not MEDIATYPE_Video.\n");
    //    return E_INVALIDARG;
    //}

    ////// Check if the subtype is our preferred L8/Y800
    ////// Note: Some renderers might propose other subtypes. You could add logic
    ////// here to handle conversions if needed, but for simplicity, we'll only
    ////// accept our preferred format.
    ////if (pMediaType->subtype != MEDIASUBTYPE_L8 &&
    ////    pMediaType->GetSubtype() != MEDIASUBTYPE_Y)
    ////{
    ////    printf("CheckMediaType: Subtype is not MEDIASUBTYPE_L8 or MEDIASUBTYPE_Y800.\n");
    ////    return E_INVALIDARG;
    ////}

    //// Check if the format type is VIDEOINFOHEADER
    //if (pMediaType->formattype != FORMAT_VideoInfo)
    //{
    //    printf("CheckMediaType: Format type is not FORMAT_VideoInfo.\n");
    //    return E_INVALIDARG;
    //}

    //// Check format details
    //VIDEOINFOHEADER* pVih = (VIDEOINFOHEADER*)pMediaType->Format();
    //if (pVih == NULL)
    //{
    //    printf("CheckMediaType: VIDEOINFOHEADER is NULL.\n");
    //    return E_INVALIDARG;
    //}

    //// Check dimensions and bit depth
    //long bytesPerPixel = 1; //pKinectInfraredCam->GetBytesPerPixel(); // Expected output bytes per pixel
    //long expectedWidth = 640;
    //long expectedHeight = 480;
    //if (expectedWidth <= 0 || expectedHeight <= 0 || bytesPerPixel <= 0) {
    //    printf("CheckMediaType: Kinect dimensions/pixel size invalid.\n");
    //    return E_UNEXPECTED; // Kinect helper not ready or invalid dimensions
    //}

    //// Check if dimensions match our expected output dimensions
    //if (pVih->bmiHeader.biWidth != expectedWidth || abs(pVih->bmiHeader.biHeight) != expectedHeight) // Use abs() for height as it can be negative
    //{
    //    printf("CheckMediaType: Dimensions do not match. Expected %dx%d, Got %dx%d.\n",
    //        expectedWidth, expectedHeight, pVih->bmiHeader.biWidth, abs(pVih->bmiHeader.biHeight));
    //    return E_INVALIDARG;
    //}

    //// Check bits per pixel (should be 8 for L8/Y800)
    //if (pVih->bmiHeader.biBitCount != (WORD)(bytesPerPixel * 8))
    //{
    //    printf("CheckMediaType: Bit depth does not match. Expected %d, Got %d.\n", bytesPerPixel * 8, pVih->bmiHeader.biBitCount);
    //    return E_INVALIDARG;
    //}

    //// Check image size (should match width * height * bytesPerPixel)
    //long expectedImageSize = expectedWidth * expectedHeight * bytesPerPixel; // Use actual dimensions and bytesperpixel
    //if (pVih->bmiHeader.biSizeImage != expectedImageSize)
    //{
    //    // Some applications might set biSizeImage to 0 for uncompressed formats,
    //    // but it's safer to check against the calculated size.
    //    if (pVih->bmiHeader.biSizeImage != 0) {
    //        printf("CheckMediaType: Image size does not match. Expected %ld, Got %ld.\n", expectedImageSize, pVih->bmiHeader.biSizeImage);
    //        // return E_INVALIDARG; // Be strict or lenient here
    //    }
    //}

    //// Check Compression (should be BI_RGB or 0 for L8/Y800)
    //if (pVih->bmiHeader.biCompression != BI_RGB && pVih->bmiHeader.biCompression != 0)
    //{
    //    printf("CheckMediaType: Compression not BI_RGB or 0.\n");
    //    return E_INVALIDARG;
    //}


    //printf("CheckMediaType: Media type accepted.\n");
    //return S_OK; // Media type is acceptable
}

HRESULT CKinectVirtualStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pRequest)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)m_mt.Format();
    pRequest->cBuffers = 1;
    pRequest->cbBuffer = pvi->bmiHeader.biSizeImage;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pRequest, &Actual);

    if (FAILED(hr)) return hr;
    if (Actual.cbBuffer < pRequest->cbBuffer) return E_FAIL;

    return NOERROR;
}

HRESULT CKinectVirtualStream::FillBuffer(IMediaSample* pSample)
{
    //DbgLog((LOG_TRACE, 3, TEXT("CKinectVirtualStream::FillBuffer")));

    REFERENCE_TIME rtNow;

    REFERENCE_TIME avgFrameTime = ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;

    rtNow = m_rtLastTime;
    m_rtLastTime += avgFrameTime;
    pSample->SetTime(&rtNow, &m_rtLastTime);
    pSample->SetSyncPoint(TRUE);

    BYTE* pData;
    long lDataLen;
    pSample->GetPointer(&pData);
    lDataLen = pSample->GetSize();
    if (m_pParent->m_kinected)
    {
        m_pParent->m_kinectRGBCam.Nui_GetCamFrame(m_pParent->m_pBuffer, m_pParent->m_pBufferSize);
        const BYTE* pSrc = m_pParent->m_pBuffer + (640 * 480 - 1) * 4;

        for (int i = 0; i < 640 * 480; ++i) {
            *pData++ = pSrc[0];
            *pData++ = pSrc[1];
            *pData++ = pSrc[2];
            pSrc -= 4;
        }
    }
    else
    {
        // Fill with random data if Kinect isn't connected
        for (int i = 0; i < lDataLen; ++i)
            *pData++ = rand();
    }

    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////


HRESULT STDMETHODCALLTYPE CKinectVirtualStream::SetFormat(AM_MEDIA_TYPE* pmt)
{
    //DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
    m_mt = *pmt;
    IPin* pin;
    ConnectedTo(&pin);
    if (pin)
    {
        IFilterGraph* pGraph = m_pParent->GetGraph();
        pGraph->Reconnect(this);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CKinectVirtualStream::GetFormat(AM_MEDIA_TYPE** ppmt)
{
    *ppmt = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CKinectVirtualStream::GetNumberOfCapabilities(int* piCount, int* piSize)
{
    *piCount = 1;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CKinectVirtualStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC)
{
    if (iIndex < 0 || iIndex > 0) return VFW_S_NO_MORE_ITEMS;
    if (pmt == NULL || pSCC == NULL) return E_POINTER;
    *pmt = CreateMediaType(&m_mt);
    if (*pmt == NULL) return E_OUTOFMEMORY;
    DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

    //    if (iIndex == 0) iIndex = 4;
    /*if (iIndex == 0) iIndex = 8;*/

    //pvi->bmiHeader.biCompression = BI_RGB;
    //pvi->bmiHeader.biBitCount = 24;
    //pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    //pvi->bmiHeader.biWidth = 640;
    //pvi->bmiHeader.biHeight = 480;
    //pvi->bmiHeader.biPlanes = 1;
    //pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
    //pvi->bmiHeader.biClrImportant = 0;

    //SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    //SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    (*pmt)->majortype = MEDIATYPE_Video;
    (*pmt)->subtype = MEDIASUBTYPE_RGB24;
    (*pmt)->formattype = FORMAT_VideoInfo;
    (*pmt)->bTemporalCompression = FALSE;
    (*pmt)->bFixedSizeSamples = FALSE;
    (*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
    (*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);

    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);

    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = 640;
    pvscc->InputSize.cy = 480;
    pvscc->MinCroppingSize.cx = 640;
    pvscc->MinCroppingSize.cy = 480;
    pvscc->MaxCroppingSize.cx = 640;
    pvscc->MaxCroppingSize.cy = 480;
    pvscc->CropGranularityX = 0;
    pvscc->CropGranularityY = 0;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;

    pvscc->MinOutputSize.cx = 640;
    pvscc->MinOutputSize.cy = 480;
    pvscc->MaxOutputSize.cx = 640;
    pvscc->MaxOutputSize.cy = 480;
    pvscc->OutputGranularityX = 0;
    pvscc->OutputGranularityY = 0;
    pvscc->StretchTapsX = 0;
    pvscc->StretchTapsY = 0;
    pvscc->ShrinkTapsX = 0;
    pvscc->ShrinkTapsY = 0;
    pvscc->MinFrameInterval = 333333;
    pvscc->MaxFrameInterval = 333333;
    pvscc->MinBitsPerSecond = 640 * 480 * 24 * 30;
    pvscc->MaxBitsPerSecond = 640 * 480 * 24 * 30;

    return S_OK;
    //*pmt = CreateMediaType(&m_mt);
    //DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

    ////    if (iIndex == 0) iIndex = 4;
    //if (iIndex == 0) iIndex = 8;

    ////pVih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    ////pVih->bmiHeader.biWidth = 640; //m_lVideoWidth;
    ////pVih->bmiHeader.biHeight = 480;//m_lVideoHeight; // Height is positive for uncompressed bottom-up DIBs, negative for top-down. DirectShow often uses bottom-up.
    ////pVih->bmiHeader.biPlanes = 1;
    ////pVih->bmiHeader.biBitCount = 16; // 8 bits per pixel for L8
    ////pVih->bmiHeader.biCompression = BI_RGB; // BI_RGB with 8bpp implies L8
    ////pVih->bmiHeader.biSizeImage = GetBitmapSize(&pVih->bmiHeader);

    //pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    //pvi->bmiHeader.biWidth = 640;
    //pvi->bmiHeader.biHeight = 480;
    //pvi->bmiHeader.biPlanes = 1;
    //pvi->bmiHeader.biBitCount = 24;
    //pvi->bmiHeader.biCompression = BI_RGB;
    //pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
    //pvi->bmiHeader.biClrImportant = 0;

    //SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    //SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    //(*pmt)->majortype = MEDIATYPE_Video;
    //(*pmt)->subtype = MEDIASUBTYPE_RGB24;
    //(*pmt)->formattype = FORMAT_VideoInfo;
    //(*pmt)->bTemporalCompression = FALSE;
    //(*pmt)->bFixedSizeSamples = FALSE;
    //(*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
    //(*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);

    //DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);

    //pvscc->guid = FORMAT_VideoInfo;
    //pvscc->VideoStandard = AnalogVideo_None;
    //pvscc->InputSize.cx = 640;
    //pvscc->InputSize.cy = 480;
    //pvscc->MinCroppingSize.cx = 640;
    //pvscc->MinCroppingSize.cy = 480;
    //pvscc->MaxCroppingSize.cx = 640;
    //pvscc->MaxCroppingSize.cy = 480;
    //pvscc->CropGranularityX = 640;
    //pvscc->CropGranularityY = 480;
    //pvscc->CropAlignX = 0;
    //pvscc->CropAlignY = 0;

    //pvscc->MinOutputSize.cx = 640;
    //pvscc->MinOutputSize.cy = 480;
    //pvscc->MaxOutputSize.cx = 640;
    //pvscc->MaxOutputSize.cy = 480;
    //pvscc->OutputGranularityX = 0;
    //pvscc->OutputGranularityY = 0;
    //pvscc->StretchTapsX = 0;
    //pvscc->StretchTapsY = 0;
    //pvscc->ShrinkTapsX = 0;
    //pvscc->ShrinkTapsY = 0;
    //pvscc->MinFrameInterval = 333333;
    //pvscc->MaxFrameInterval = 333333;
    //pvscc->MinBitsPerSecond = 640 * 480 * 16 * 30;
    //pvscc->MaxBitsPerSecond = 640 * 480 * 16 * 30;

    //return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//  IKsPropertySet
//////////////////////////////////////////////////////////////////////////

HRESULT CKinectVirtualStream::Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData,
    DWORD cbInstanceData, void* pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CKinectVirtualStream::Get(
    REFGUID guidPropSet,   // Which property set.
    DWORD dwPropID,        // Which property in that set.
    void* pInstanceData,   // Instance data (ignore).
    DWORD cbInstanceData,  // Size of the instance data (ignore).
    void* pPropData,       // Buffer to receive the property data.
    DWORD cbPropData,      // Size of the buffer.
    DWORD* pcbReturned     // Return the size of the property.
)
{
    if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
    if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;

    if (pcbReturned) *pcbReturned = sizeof(GUID);
    if (pPropData == NULL)          return S_OK; // Caller just wants to know the size. 
    if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;// The buffer is too small.

    *(GUID*)pPropData = PIN_CATEGORY_CAPTURE;
    return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CKinectVirtualStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    // We support getting this property, but not setting it.
    if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET;
    return S_OK;
}