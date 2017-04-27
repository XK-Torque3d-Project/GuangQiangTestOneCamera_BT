
#pragma include_alias( "dxtrans.h", "qedit.h" )

#define __IDxtCompositor_INTERFACE_DEFINED__

#define __IDxtAlphaSetter_INTERFACE_DEFINED__

#define __IDxtJpeg_INTERFACE_DEFINED__

#define __IDxtKey_INTERFACE_DEFINED__

//#include <streams.h>
//#include <atlbase.h>
#include <qedit.h>
#include <vector>
#include <algorithm>
#include "PlayCapRectify.h"

#include "console/console.h"

GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR           gdiplusToken;

static HANDLE g_hAnd = NULL;
static HANDLE m_hUSBDevice = NULL;
//HWND g_hCalibrationWnd = NULL;

IVideoWindow  * g_pVW[1] = {0};
IMediaControl * g_pMC[1] = {0};
IMediaEventEx * g_pME[1] = {0};
IGraphBuilder * g_pGraph[1] = {0};
ICaptureGraphBuilder2 * g_pCapture[1] = {0};

CSampleGrabberCB *CB = NULL;
CSampleGrabberCB *CB1 = NULL;

bool restartCamera = false;

#ifdef debug
extern HWND g_hApp;
#endif

void Msg(TCHAR *szFormat, ...)
{
	TCHAR szBuffer[1024]; 
	const size_t NUMCHARS = sizeof(szBuffer) / sizeof(szBuffer[0]);
	const int LASTCHAR = NUMCHARS - 1;

	va_list pArgs;
	va_start(pArgs, szFormat);
	(void)StringCchVPrintf(szBuffer, NUMCHARS - 1, szFormat, pArgs);
	va_end(pArgs);

	szBuffer[LASTCHAR] = TEXT('\0');
	MessageBox(NULL,  szBuffer, L"error!", MB_OK);
}

HRESULT CaptureVideo()
{
    HRESULT hr = S_FALSE;
	IBaseFilter *pSrcFilter[1]={NULL}; 
    
	CB = new CSampleGrabberCB(ID_CAMERA1);
	//CB1 = new CSampleGrabberCB(ID_CAMERA2);

	//if(CB == NULL || CB1 == NULL)
	if(CB == NULL)
	{
		//Msg(TEXT("Failed to get CB CB1!"), NULL);
		Msg(TEXT("Failed to get CB!"), NULL);
		return hr;
	}

	hr = GetInterfaces();
	if (FAILED(hr))
	{
		Msg(TEXT("Failed to get video interfaces!  hr=0x%x"), hr);
		return hr;
	}

	hr = g_pCapture[0]->SetFiltergraph(g_pGraph[0]);
	if (FAILED(hr))
	{
		Msg(TEXT("Failed to set capture filter graph!  hr=0x%x"), hr);
		return hr;
	}

	//hr = g_pCapture[1]->SetFiltergraph(g_pGraph[1]);
	//if (FAILED(hr))
	//{
	//	//Msg(TEXT("Failed to set capture filter graph!  hr=0x%x"), hr);
	//	return hr;
	//}

	hr = FindCaptureDevice(pSrcFilter);
	if (FAILED(hr))
	{
		Msg(TEXT("Failed to find capture filter!  hr=0x%x"), hr);
		return hr;
	}

	hr = g_pGraph[0]->AddFilter(pSrcFilter[0], L"Video Capture0");
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't add the capture filter to the graph!  hr=0x%x\r\n\r\n") 
			TEXT("If you have a working video capture device, please make sure\r\n")
			TEXT("that it is connected and is not being used by another application.\r\n\r\n")
			TEXT("The sample will now close."), hr);
		pSrcFilter[0]->Release();
		return hr;
	}

	/*if( pSrcFilter[ 1 ] )
	{
		hr = g_pGraph[1]->AddFilter(pSrcFilter[1], L"Video Capture1");
		if (FAILED(hr))
		{
			Msg(TEXT("Couldn't add the capture filter to the graph!  hr=0x%x\r\n\r\n") 
				TEXT("If you have a working video capture device, please make sure\r\n")
				TEXT("that it is connected and is not being used by another application.\r\n\r\n")
				TEXT("The sample will now close."), hr);
			pSrcFilter[0]->Release();
			pSrcFilter[1]->Release();
			return hr;
		}
	}
	else
	{
		Msg(TEXT("Couldn't add the capture filter to the graph! pSrcFilter[1] hr=0x%x\r\n\r\n") 
			TEXT("If you have a working video capture device, please make sure\r\n")
			TEXT("that it is connected and is not being used by another application.\r\n\r\n")
			TEXT("The sample will now close."), hr);
	}*/

	IBaseFilter *pF[1] = {NULL};
	ISampleGrabber *pGrabber[1] = {NULL};

	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, reinterpret_cast<void**>(&pF[0]));
	if( FAILED( hr ) )
	{
		Msg(TEXT("Couldn't create sample grabber filter!  hr=0x%x\r\n\r\n"), hr);
		pF[0]->Release();
		pSrcFilter[0]->Release();
		//pSrcFilter[1]->Release();
		return hr;
	}

	/*if( pSrcFilter[ 1 ] )
	{
		hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
			IID_IBaseFilter, reinterpret_cast<void**>(&pF[1]));
		if( FAILED( hr ) )
		{
			Msg(TEXT("Couldn't create sample grabber filter!  hr=0x%x\r\n\r\n"), hr);
			pF[0]->Release();
			pF[1]->Release();
			pSrcFilter[0]->Release();
			pSrcFilter[1]->Release();
			return hr;
		}
	}*/

	hr = pF[0]->QueryInterface(IID_ISampleGrabber, reinterpret_cast<void**>(&pGrabber[0]));
	if( FAILED( hr ) )
	{
		Msg(TEXT("Couldn't create sample grabber filter!  hr=0x%x\r\n\r\n"), hr);
		pGrabber[0]->Release();
		pF[0]->Release();
		//pF[1]->Release();
		pSrcFilter[0]->Release();
		//pSrcFilter[1]->Release();
		return hr;
	}

	/*if( pSrcFilter[ 1 ] )
	{
		hr = pF[1]->QueryInterface(IID_ISampleGrabber,
			reinterpret_cast<void**>(&pGrabber[1]));
		if( FAILED( hr ) )
		{
			Msg(TEXT("Couldn't create sample grabber filter!  hr=0x%x\r\n\r\n"), hr);
			pGrabber[0]->Release();
			pGrabber[1]->Release();
			pF[0]->Release();
			pF[1]->Release();
			pSrcFilter[0]->Release();
			pSrcFilter[1]->Release();
			return hr;
		}
	}*/

	hr = g_pGraph[0]->AddFilter(pF[0], L"SampleGrabber0");
	if( FAILED( hr ) )
	{
		Msg(TEXT("Couldn't add the grabber filter to the graph!  hr=0x%x\r\n\r\n") 
			TEXT("If you have a working video capture device, please make sure\r\n")
			TEXT("that it is connected and is not being used by another application.\r\n\r\n")
			TEXT("The sample will now close."), hr);
		pGrabber[0]->Release();
		//pGrabber[1]->Release();
		pF[0]->Release();
		//pF[1]->Release();
		pSrcFilter[0]->Release();
		//pSrcFilter[1]->Release();
		return hr;
	}

	/*if( pSrcFilter[ 1 ] )
	{
		hr = g_pGraph[1]->AddFilter(pF[1], L"SampleGrabber1");
		if( FAILED( hr ) )
		{
			Msg(TEXT("Couldn't add the grabber filter to the graph!  hr=0x%x\r\n\r\n") 
				TEXT("If you have a working video capture device, please make sure\r\n")
				TEXT("that it is connected and is not being used by another application.\r\n\r\n")
				TEXT("The sample will now close."), hr);
			pGrabber[0]->Release();
			pGrabber[1]->Release();
			pF[0]->Release();
			pF[1]->Release();
			pSrcFilter[0]->Release();
			pSrcFilter[1]->Release();
			return hr;
		}
	}*/

	HDC hdc = GetDC(NULL);
	int iBitDepth = GetDeviceCaps(hdc, BITSPIXEL); 
	ReleaseDC(NULL, hdc);

	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
	mt.majortype = MEDIATYPE_Video;
	switch (iBitDepth)
	{
	case 8:
		mt.subtype = MEDIASUBTYPE_RGB8;
		break;
	case 16:
		mt.subtype = MEDIASUBTYPE_RGB555;
		break;
	case 24:
		mt.subtype = MEDIASUBTYPE_RGB24;
		break;
	case 32:
		mt.subtype = MEDIASUBTYPE_RGB24;
		break;
	default:
		return E_FAIL;
	}

	hr = pGrabber[0]->SetMediaType(&mt);
	if( FAILED( hr ) )
	{
		Msg( TEXT( "Couldn't set media type!  hr=0x%x\r\n\r\n" ), hr );
		pGrabber[0]->Release();
		//pGrabber[1]->Release();
		pF[0]->Release();
		//pF[1]->Release();
		pSrcFilter[0]->Release();
		//pSrcFilter[1]->Release();
		return hr;
	}

	/*if( pSrcFilter[ 1 ] )
	{
		hr = pGrabber[1]->SetMediaType(&mt);
		if( FAILED( hr ) )
		{
			Msg( TEXT( "Couldn't set media type!  hr=0x%x\r\n\r\n" ), hr );
			pGrabber[0]->Release();
			pGrabber[1]->Release();
			pF[0]->Release();
			pF[1]->Release();
			pSrcFilter[0]->Release();
			pSrcFilter[1]->Release();
			return hr;
		}
	}*/

	IBaseFilter *pNull[1] = {NULL};
	hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, reinterpret_cast<void**>(&pNull[0]));
	if( FAILED( hr ) )
	{
		Msg( TEXT( "Couldn't create null renderer filter!  hr=0x%x\r\n\r\n" ), hr );
		pGrabber[0]->Release();
		//pGrabber[1]->Release();
		pF[0]->Release();
		//pF[1]->Release();
		pSrcFilter[0]->Release();
		//pSrcFilter[1]->Release();
		return hr;
	}

	/*if( pSrcFilter[ 1 ] )
	{
		hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
			IID_IBaseFilter, reinterpret_cast<void**>(&pNull[1]));
		if( FAILED( hr ) )
		{
			Msg( TEXT( "Couldn't create null renderer filter!  hr=0x%x\r\n\r\n" ), hr );
			pGrabber[0]->Release();
			pGrabber[1]->Release();
			pF[0]->Release();
			pF[1]->Release();
			pSrcFilter[0]->Release();
			pSrcFilter[1]->Release();
			return hr;
		}
	}*/

	hr = g_pGraph[0]->AddFilter(pNull[0], L"NullRenderer0");
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't add the pNull filter to the graph!  hr=0x%x\r\n\r\n") 
			TEXT("If you have a working video capture device, please make sure\r\n")
			TEXT("that it is connected and is not being used by another application.\r\n\r\n")
			TEXT("The sample will now close."), hr);
		pGrabber[0]->Release();
		//pGrabber[1]->Release();
		pF[0]->Release();
		//pF[1]->Release();
		pSrcFilter[0]->Release();
		//pSrcFilter[1]->Release();
		pNull[0]->Release();
		//pNull[1]->Release();
		return hr;
	}

	/*if( pSrcFilter[ 1 ] )
	{
		hr = g_pGraph[1]->AddFilter(pNull[1], L"NullRenderer1");
		if (FAILED(hr))
		{
			Msg(TEXT("Couldn't add the pNull filter to the graph!  hr=0x%x\r\n\r\n") 
				TEXT("If you have a working video capture device, please make sure\r\n")
				TEXT("that it is connected and is not being used by another application.\r\n\r\n")
				TEXT("The sample will now close."), hr);
			pGrabber[0]->Release();
			pGrabber[1]->Release();
			pF[0]->Release();
			pF[1]->Release();
			pSrcFilter[0]->Release();
			pSrcFilter[1]->Release();
			pNull[0]->Release();
			pNull[1]->Release();
			return hr;
		}
	}*/

	hr = g_pCapture[0]->RenderStream( &PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
		pSrcFilter[0], pF[0], pNull[0]);
	if (FAILED(hr))
	{
		Msg(TEXT("Couldn't render the video capture stream. 0  hr=0x%x\r\n")
			TEXT("The capture device may already be in use by another application.\r\n\r\n")
			TEXT("The sample will now close."), hr);
		pGrabber[0]->Release();
		//pGrabber[1]->Release();
		pF[0]->Release();
		//pF[1]->Release();
		pSrcFilter[0]->Release();
		//pSrcFilter[1]->Release();
		pNull[0]->Release();
		//pNull[1]->Release();
		return hr;
	}

	/*if( pSrcFilter[ 1 ] )
	{
		hr = g_pCapture[1]->RenderStream( &PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
			pSrcFilter[1], pF[1], pNull[1]);

		if (FAILED(hr))
		{
			Msg(TEXT("Couldn't render the video capture stream. 1 hr=0x%x\r\n")
				TEXT("The capture device may already be in use by another application.\r\n\r\n")
				TEXT("The sample will now close."), hr);
			pGrabber[0]->Release();
			pGrabber[1]->Release();
			pF[0]->Release();
			pF[1]->Release();
			pSrcFilter[0]->Release();
			pSrcFilter[1]->Release();
			pNull[0]->Release();
			pNull[1]->Release();
			return hr;
		}
	}*/

	AM_MEDIA_TYPE mt1;
	hr = pGrabber[0]->GetConnectedMediaType( &mt1 );
	if( hr == S_OK )
	{
		VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) mt1.pbFormat;
		CB->Width  = vih->bmiHeader.biWidth;
		CB->Height = vih->bmiHeader.biHeight;

		CB->image = cvCreateImage(cvSize( CB->Width, CB->Height ), IPL_DEPTH_8U, 1);
		CB->image->origin = 1;
#ifdef debug
		SetTimer( g_hApp, 1001, 1000, OnTimer );
		hwnd = g_hApp; 
#endif

		pGrabber[0]->SetBufferSamples( TRUE );
		pGrabber[0]->SetOneShot( FALSE );
		pGrabber[0]->SetCallback(CB, 1);	
	}

//	if( pSrcFilter[ 1 ] )
//	{
//		hr = pGrabber[1]->GetConnectedMediaType( &mt1 );
//        
//		if( hr == S_OK )
//		{
//			VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) mt1.pbFormat;
//			CB1->Width  = vih->bmiHeader.biWidth;
//			CB1->Height = vih->bmiHeader.biHeight;
//
//			CB1->image = cvCreateImage(cvSize( CB->Width, CB->Height ), IPL_DEPTH_8U, 1);
//			CB1->image->origin = 1;
//#ifdef debug
//			hwnd = g_hSubApp;
//#endif
//			pGrabber[1]->SetBufferSamples( TRUE );
//			pGrabber[1]->SetOneShot( FALSE );
//			pGrabber[1]->SetCallback(CB1, 1);
//		}
//	}

	SAFE_RELEASE( pGrabber[0] );
	//SAFE_RELEASE( pGrabber[1] );
	SAFE_RELEASE( pF[0] );
	//SAFE_RELEASE( pF[1] );
	SAFE_RELEASE( pNull[0] );
	//SAFE_RELEASE( pNull[1] );
	SAFE_RELEASE( pSrcFilter[0] );
	//SAFE_RELEASE( pSrcFilter[1] );

//#ifdef debug
//	SetupVideoWindow();
//#endif
 
	hr = g_pMC[0]->Stop();
	if ( FAILED( hr ) )
	{
		Msg(TEXT("Couldn't stop the graph!  hr=0x%x"), hr);
		return hr;
	}

	//hr = g_pMC[1]->Stop();
	//if ( FAILED( hr ) )
	//{
	//	//Msg(TEXT("Couldn't run the graph!  hr=0x%x"), hr);
	//	return hr;
	//}
	return S_OK;
}

HRESULT GetInterfaces(void)
{
	HRESULT hr = S_FALSE;
	hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC,
		IID_IGraphBuilder, (void **) &g_pGraph[0]);
	if (FAILED(hr))
		return hr;

	hr = CoCreateInstance (CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC,
		IID_ICaptureGraphBuilder2, (void **) &g_pCapture[0]);
	if (FAILED(hr))
		return hr;

	hr = g_pGraph[0]->QueryInterface(IID_IMediaControl,(LPVOID *) &g_pMC[0]);
	if (FAILED(hr))
		return hr;

	hr = g_pGraph[0]->QueryInterface(IID_IVideoWindow, (LPVOID *) &g_pVW[0]);
	if (FAILED(hr))
		return hr;

	hr = g_pGraph[0]->QueryInterface(IID_IMediaEvent, (LPVOID *) &g_pME[0]);
	if (FAILED(hr))
		return hr;

	/*hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC,
		IID_IGraphBuilder, (void **) &g_pGraph[1]);
	if (FAILED(hr))
		return hr;

	hr = CoCreateInstance (CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC,
		IID_ICaptureGraphBuilder2, (void **) &g_pCapture[1]);
	if (FAILED(hr))
		return hr;

	hr = g_pGraph[1]->QueryInterface(IID_IMediaControl,(LPVOID *) &g_pMC[1]);
	if (FAILED(hr))
		return hr;

	hr = g_pGraph[1]->QueryInterface(IID_IVideoWindow, (LPVOID *) &g_pVW[1]);
	if (FAILED(hr))
		return hr;

	hr = g_pGraph[1]->QueryInterface(IID_IMediaEvent, (LPVOID *) &g_pME[1]);
	if (FAILED(hr))
		return hr;*/

	return hr;
}


void CloseInterfaces(void)
{
	if (g_pMC[0])
		g_pMC[0]->StopWhenReady();

	/*if (g_pMC[1])
		g_pMC[1]->StopWhenReady();*/

	if(g_pVW[0])
	{
		g_pVW[0]->put_Visible(OAFALSE);
		g_pVW[0]->put_Owner(NULL);
	}

	/*if(g_pVW[1])
	{
		g_pVW[1]->put_Visible(OAFALSE);
		g_pVW[1]->put_Owner(NULL);
	}*/

//#ifdef REGISTER_FILTERGRAPH
//	// Remove filter graph from the running object table   
//	if (g_dwGraphRegister)
//		RemoveGraphFromRot(g_dwGraphRegister);
//#endif

	if ( !restartCamera )
	{
		SAFE_RELEASE(g_pMC[0]);
		SAFE_RELEASE(g_pME[0]);
		SAFE_RELEASE(g_pVW[0]);
		SAFE_RELEASE(g_pGraph[0]);
		SAFE_RELEASE(g_pCapture[0]);

		/*SAFE_RELEASE(g_pMC[1]);
		SAFE_RELEASE(g_pME[1]);
		SAFE_RELEASE(g_pVW[1]);
		SAFE_RELEASE(g_pGraph[1]);
		SAFE_RELEASE(g_pCapture[1]);*/
	}
}

HRESULT FindCaptureDevice(IBaseFilter ** ppSrcFilter)
{
	HRESULT hr = S_OK;
	IBaseFilter * pSrc[1] = {NULL};
	IMoniker* pMoniker =NULL;
	ICreateDevEnum *pDevEnum =NULL;
	IEnumMoniker *pClassEnum = NULL;

	if (!ppSrcFilter)
	{
		return E_POINTER;
	}

	hr = CoCreateInstance (CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
		IID_ICreateDevEnum, (void **) &pDevEnum);
	if (FAILED(hr))
	{
		//Msg(TEXT("Couldn't create system enumerator!  hr=0x%x"), hr);
	}

	if (SUCCEEDED(hr))
	{
		hr = pDevEnum->CreateClassEnumerator (CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
		if (FAILED(hr))
		{
			//Msg(TEXT("Couldn't create class enumerator!  hr=0x%x"), hr);
		}
	}

	if (SUCCEEDED(hr))
	{
		if (pClassEnum == NULL)
		{
			//MessageBox(NULL,TEXT("No video capture device was detected.\r\n\r\n")
			//	TEXT("This sample requires a video capture device, such as a USB WebCam,\r\n")
			//	TEXT("to be installed and working properly.  The sample will now close."),
			//	TEXT("No Video Capture Hardware"), MB_OK | MB_ICONINFORMATION);
			hr = E_FAIL;
		}
	}

	if (hr != S_OK)
	{
		Con::executef( "OnConnectState", "5" );
		//Con::executef( "OnConnectState", "7" );
	}

	for( int i = 0; i < 1 && hr == S_OK; i++ )
	{	
		if (SUCCEEDED(hr))
		{
			hr = pClassEnum->Next (1, &pMoniker, NULL);
			if (hr == S_FALSE)
			{
				if( !pSrc[ 0 ] )
					Msg(TEXT("Unable to access video capture device!"));
				hr = E_FAIL;
			}	
		}

		if (SUCCEEDED(hr))
		{
			IBindCtx * pbc = NULL;
			IMalloc * pmalloc = NULL;
			WCHAR * pDisplayName = NULL;

			WCHAR wPidStr[10], wVidStr[10];
			int i = 0;

			CoGetMalloc( 1, &pmalloc );
			CreateBindCtx( 0, &pbc );

			pMoniker->GetDisplayName( pbc, NULL, &pDisplayName );

			while(pDisplayName[i] != '\0')
			{
				if(pDisplayName[i] == 'v' && pDisplayName[i+1] == 'i' && pDisplayName[i+2] == 'd' && pDisplayName[i+3] == '_')
				{
					int j = 0;

					for(i += 4; pDisplayName[i] != '&'; i++)
					{
						wVidStr[j++] = pDisplayName[i];
					}

					wVidStr[j] = '\0';
				}

				if(pDisplayName[i] == 'p' && pDisplayName[i+1] == 'i' && pDisplayName[i+2] == 'd' && pDisplayName[i+3] == '_')
				{
					int j = 0;

					for(i += 4; pDisplayName[i] != '&'; i++)
					{
						wPidStr[j++] = pDisplayName[i];
					}
                    
					wPidStr[j] = '\0';
					break;
				}
				++i;
			}

			pmalloc->Free( pDisplayName );
			pmalloc->Release();
			pbc->Release();
			int result = wcscmp(wVidStr, L"04fc");
			if(result != 0)
			{
				//Msg(TEXT("Not video capture device!"));
				exit(1);
			}

            if(wcscmp(wPidStr, L"fa02") == 0 || wcscmp(wPidStr, L"fa09") == 0)
			{
				hr = pMoniker->BindToObject(0,0,IID_IBaseFilter, (void**)&pSrc[0]);
				if (FAILED(hr))
				{
					Msg(TEXT("Couldn't bind moniker to filter object!  hr=0x%x"), hr);
				}

				if (SUCCEEDED(hr))
				{
					ppSrcFilter[0] = pSrc[0];
					ppSrcFilter[0]->AddRef();
				}
				Con::executef( "OnConnectState", "4" );
			}

			/*if(wcscmp(wPidStr, L"fa06") == 0)
			{
				hr = pMoniker->BindToObject(0,0,IID_IBaseFilter, (void**)&pSrc[1]);
				if (FAILED(hr))
				{ 
					Msg(TEXT("Couldn't bind moniker to filter object!  hr=0x%x"), hr);
				}

				if (SUCCEEDED(hr))
				{
					ppSrcFilter[1] = pSrc[1];
					ppSrcFilter[1]->AddRef();
				}
				Con::executef( "OnConnectState", "6" );
			}*/

			if(wcscmp(wPidStr, L"fa02") != 0 && wcscmp(wPidStr, L"fa09") != 0)
			{
				MessageBox(NULL,  L"Camera id error!", L"error!", MB_OK);
			}
		}
	}

	if( pSrc[ 0 ] )
		hr = S_OK;

	SAFE_RELEASE(pSrc[0]);
	//SAFE_RELEASE(pSrc[1]);
	SAFE_RELEASE(pMoniker);
	SAFE_RELEASE(pDevEnum);
	SAFE_RELEASE(pClassEnum);
	return hr;
}

//HRESULT SetupVideoWindow(void)
//{
//	//HRESULT hr = NULL;
//    
//	//hr = g_pVW->put_Owner((OAHWND)g_hApp);
//	//if (FAILED(hr))
//	//	return hr;
//
//	//hr = g_pVW->put_MessageDrain( (OAHWND)g_hApp );
//
//	//hr = g_pVW->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
//	//if (FAILED(hr))
//	//	return hr;
//
//	//ResizeVideoWindow();
//
//	//hr = g_pVW->put_Visible(OATRUE);
//	//if (FAILED(hr))
//	//	return hr;
//
//	return S_OK;
//}

//void ResizeVideoWindow(void)
//{
//	//if (g_pVW)
//	//{
//	//	RECT rc;
//
//	//	g_pVW->SetWindowPosition(0, 0, rc.right, rc.bottom);
//	//}
//}

//HRESULT HandleGraphEvent(void)
//{
//    HRESULT hr=S_OK;
//
//    //if (!g_pME)
//    //    return E_POINTER;
//
//    //while(SUCCEEDED(g_pME->GetEvent(&evCode, &evParam1, &evParam2, 0)))
//    //{
//    //    hr = g_pME->FreeEventParams(evCode, evParam1, evParam2);
//    //    
//    //}
//
//    return hr;
//}

WCHAR szWindowClass[] = L"CalibrationForm";
WCHAR szTitle[] = L"form";



//ATOM MyRegisterClass(HINSTANCE hInstance)
//{
//	WNDCLASSEX wcex;
//
//	wcex.cbSize = sizeof(WNDCLASSEX);
//
//	wcex.style			= CS_HREDRAW | CS_VREDRAW;
//	wcex.lpfnWndProc	= CalibrationWndProc;
//	wcex.cbClsExtra		= 0;
//	wcex.cbWndExtra		= 0;
//	wcex.hInstance		= hInstance;
//	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTWINDOW123));
//	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
//	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
//	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TESTWINDOW123);
//	wcex.lpszClassName	= szWindowClass;
//	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
//
//	return RegisterClassEx(&wcex);
//}

//int unRegisterMyclass()
//{
//	UnregisterClass(szWindowClass, NULL);
//
//	return 1;
//}

//int InitInstance(HINSTANCE hInstance)
//{
//	HWND hWnd;
//
//	/*UINT width = GetSystemMetrics(SM_CXSCREEN);
//	UINT height = GetSystemMetrics(SM_CYSCREEN);*/
//
//	hWnd=CreateWindow(
//		szWindowClass,
//		szTitle,
//		WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_CLIPCHILDREN /*WS_POPUP*/,
//		300,100,
//		1024,768,
//		NULL,NULL,
//		hInstance,
//		NULL);
//
//	if (!hWnd)
//	{
//		return FALSE;
//	}
//
//	ShowWindow(hWnd, SW_SHOW);
//	UpdateWindow(hWnd);
//	//SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
//
//	if(g_hCalibrationWnd != NULL)
//	{
//		SendMessage(g_hCalibrationWnd, WM_CLOSE, 0, 0);
//		DestroyWindow(g_hCalibrationWnd);
//		g_hCalibrationWnd =	NULL;
//	}
//
//	g_hCalibrationWnd = hWnd;
//
//	return 1;//(int) msg.wParam;
//}

//LRESULT CALLBACK CalibrationWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//
//	//switch(message)
//	//{
//
//	//default: 
//	//	break;
//	//}
//	return DefWindowProc(hWnd, message, wParam, lParam);
//}

bool RunGun()
{
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	if(FAILED(CoInitialize(NULL)))
	{
		MessageBox(NULL, L"Can't init COM!", L"error", MB_OK);
		exit(1);
	}

	HRESULT hr = CaptureVideo();
	if (FAILED (hr))
	{
		CloseGun();
		MessageBox(NULL,  L"Camera error!", L"error!", MB_OK);
		return false;
	}
	return true;
}


int StopVideo()
{
	HRESULT hr = g_pMC[0]->Pause();
	if ( FAILED( hr ) )
	{
		//Msg(TEXT("Couldn't run the graph!  hr=0x%x"), hr);
		return hr;
	}

	//hr = g_pMC[1]->Pause();
	//if ( FAILED( hr ) )
	//{
	//	//Msg(TEXT("Couldn't run the graph!  hr=0x%x"), hr);
	//	return hr;
	//}

	return 1;
}

int PlayVideo()
{
	HRESULT hr = g_pMC[0]->Run();
	if ( FAILED( hr ) )
	{
		Msg(TEXT("Couldn't run the graph!  hr=0x%x"), hr);
		return hr;
	}

	/*hr = g_pMC[1]->Run();
	if ( FAILED( hr ) )
	{
		Msg(TEXT("Couldn't run the graph!  hr=0x%x"), hr);
		return hr;
	}*/
	return 1;
}

CSampleGrabberCB* SampleGrabberFun(int id)
{
	if(ID_CAMERA1 == id)
	{
		if (CB)
		{
		//	MessageBox(NULL, L"CB1", L"OK", MB_OK);
			return CB;
		}
		else
		{
			MessageBox(NULL , L"camera1 error!", L"error", MB_OK);
			return NULL;
		}
	}

	/*if(ID_CAMERA2 == id)
	{
		if (CB1)
		{
			return CB1;
		}
		else
		{
			MessageBox(NULL , L"camera2 error!" , L"error", MB_OK);
			return NULL;
		}
	}*/
	return NULL;
}

void CloseGun()
{
	CloseInterfaces();

	CoUninitialize();

	GdiplusShutdown(gdiplusToken);

	if(CB != NULL)
	{
		delete CB;
		CB = NULL;
		//MessageBox(NULL, L"CB release!", L"CB delete", MB_OK);
	}

	//if(CB1 != NULL)
	//{
	//	delete CB1;
	//	CB1 = NULL;
	//	//MessageBox(NULL, L"CB1 release!", L"CB1 delete", MB_OK);
	//}
}
