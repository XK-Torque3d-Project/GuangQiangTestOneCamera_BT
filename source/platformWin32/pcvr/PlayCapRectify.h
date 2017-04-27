#ifndef _PLAYCAPRECTIFY_H
#define _PLAYCAPRECTIFY_H

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <dshow.h>

#include "SampleGrabberCB.h"

//#define IDI_TESTWINDOW123			107
//#define IDI_SMALL				    108
//#define IDC_TESTWINDOW123			109

#include <gdiplus.h>
using namespace Gdiplus;

#pragma comment( lib, "gdiplus.lib" )
#pragma comment(lib,"strmiids.lib")
#pragma comment(lib,"cxcore.lib")
#pragma comment(lib,"cv.lib")
//#pragma comment(lib,"SiUSBXp.lib")

//#define   INTORF            1          //进入校准
//#define   RFPLAY            1          //校正状态
//#define   RFEXIT            2          //校正完退出状态

//#define debug

#define WM_GRAPHNOTIFY  WM_APP+1

#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }

extern bool restartCamera;

HRESULT GetInterfaces(void);
HRESULT CaptureVideo();
HRESULT FindCaptureDevice(IBaseFilter ** ppSrcFilter);
//HRESULT SetupVideoWindow(void);
//HRESULT ChangePreviewState(int nShow);
//HRESULT HandleGraphEvent(void);

void Msg(TCHAR *szFormat, ...);
void CloseInterfaces(void);
//void ResizeVideoWindow();

//HRESULT AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
//void RemoveGraphFromRot(DWORD pdwRegister);

//void DrawTarget( HDC hDC,int left,  int top, int right,  int bottom );
//extern HWND g_hCalibrationWnd;
//LRESULT CALLBACK CalibrationWndProc(HWND, UINT, WPARAM, LPARAM);
//ATOM MyRegisterClass(HINSTANCE hInstance);
//BOOL InitInstance(HINSTANCE hInstance);

//DWORD WINAPI USB_Write_Read_ThreadProc( LPVOID lpParameter);
//int InitUSB();
//int CreateUSBthread();
//void CloseUSBthread();
//int unRegisterMyclass();

bool RunGun();
void CloseGun();

int StopVideo();
int PlayVideo();

//int GetRectifyState();   
CSampleGrabberCB* SampleGrabberFun(int id);

#endif