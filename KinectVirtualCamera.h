#pragma once

#include "KinectInfraredCam.h"

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

EXTERN_C const GUID CLSID_VirtualCam;
const WCHAR g_wszFilterName[] = L"Kinect Cam";

class CKinectVirtualSource : public CSource {
public:
	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* phr);
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);

	IFilterGraph* GetGraph() { return m_pGraph; }

	KinectInfraredCam m_kinectInfraredCam;
	bool m_kinected;
	BYTE *m_pBuffer;
	int m_pBufferSize;
private:
	CKinectVirtualSource(LPUNKNOWN pUnk, HRESULT* phr);
	~CKinectVirtualSource();
};

class CKinectVirtualStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet 
{
public:
	//////////////////////////////////////////////////////////////////////////
	//  IUnknown
	//////////////////////////////////////////////////////////////////////////
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }
	STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }


	//////////////////////////////////////////////////////////////////////////
	//  IAMStreamConfig
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE* pmt);
	HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE** ppmt);
	HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int* piCount, int* piSize);
	HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC);

	//////////////////////////////////////////////////////////////////////////
	//  IKsPropertySet
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData, DWORD cbInstanceData, void* pPropData, DWORD cbPropData);
	HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void* pInstanceData, DWORD cbInstanceData, void* pPropData, DWORD cbPropData, DWORD* pcbReturned);
	HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport);

	//////////////////////////////////////////////////////////////////////////
	//  CSourceStream
	//////////////////////////////////////////////////////////////////////////
	CKinectVirtualStream(HRESULT* phr, CKinectVirtualSource* pParent, LPCWSTR pPinName);
	HRESULT FillBuffer(IMediaSample* pSample);
	HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pRequest);
	HRESULT CheckMediaType(const CMediaType* pMediaType);
	HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
	HRESULT SetMediaType(const CMediaType* pmt);
	HRESULT OnThreadCreate(void);
	HRESULT OnThreadDestroy(void);
	
private:
	CKinectVirtualSource* m_pParent;
	REFERENCE_TIME m_rtLastTime;
	HBITMAP m_hLogoBmp;
	CCritSec m_cSharedState;
	IReferenceClock* m_pClock;
};