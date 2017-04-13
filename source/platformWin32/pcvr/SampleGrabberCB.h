

#ifndef _SAMPLEGRABBERCB_H_
#define _SAMPLEGRABBERCB_H_

#pragma include_alias( "dxtrans.h", "qedit.h" )

#define __IDxtCompositor_INTERFACE_DEFINED__

#define __IDxtAlphaSetter_INTERFACE_DEFINED__

#define __IDxtJpeg_INTERFACE_DEFINED__

#define __IDxtKey_INTERFACE_DEFINED__

#include <qedit.h>
#include "State.h"
#include "MyPoint.h"

#ifdef _CH_
#pragma package <opencv>
#endif

// Debug Output
// ====================================
#define  DEBUG_OK 1
// ====================================

#ifndef _EiC
#include "cv.h"
#endif

typedef void (WINAPI *ashPINTPROC)(int,POINT);

#ifdef debug
extern HWND g_hApp;
extern HWND g_hSubApp;
extern HWND g_hShowImage1;
extern HWND g_hShowImage2;
#endif


extern int balance;


class CSampleGrabberCB : public ISampleGrabberCB 
{

private:
	UINT _ID;
public:
	int m_nGrayThreshold;
	int m_nMoveRadius;	

	long Width;
	long Height;

	long lClientWidth;
	long lClientHeight;

	int m_nBrightDotCount;
	int m_PNumber;
	int m_nSmoothPoints, m_nSmoothingCount;

	float m_fSmoothingX[20],  m_fSmoothingY[20];

	bool m_bSmoothState;

	float m_fMark;
	float m_fExsmothX, m_fExsmothY;

	MyPoint m_curMousePoint;

	long m_lTickCount;

	bool m_bSwitch;

	MODE m_mode;

	long m_lLastFrameNumber;

	long m_lFps;

	Point m_pointMouseDown;
	Point m_pointMouseMove;

	Point m_pointLight, m_pointLight1;

	Point m_p4[ 4 ];

	bool m_bYellowCon;
	int m_nYellowIndex;

	CvMat * m_translate;

	bool m_bConform;

	Warper m_warp;

	IplImage *image;

	BOOL g_bBeginDrawRectangle;

	BOOL g_bled;

	int m_nFirstInst;

	int m_nLed;

	int m_bRectifyState;
	BOOL m_bCurPointModified;

	int m_nLightcount, m_ncount, m_nPointToConvert;
	int m_ntempled;

	bool b_getUnwantedLightSource;
	Point unwantedPoint[76800];
	long unwantedPointNum;
	int getFrameNum;


	ashPINTPROC m_funPointProc;

	enum CALIBRATION_STATE
	{
		STATE_FIRST,
		STATE_SECOND,
		SATTE_THIRD,
		STATE_FOURTH,
	};

public:

	CSampleGrabberCB( UINT camid );

	STDMETHODIMP_(ULONG) AddRef() { return 2; }
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);

	STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample );

	STDMETHODIMP BufferCB( double SampleTime, BYTE * pBuffer, long BufferSize );

	void InitRectifyCfg();

	void DrawQuadrilateral(	IN Point* fourPoint, IN INT count, Color &color, BYTE * pTargetBuffer );

	void DisplayRectifyImage(BYTE * pBuffer, long BuferSize);

	void Convert2GrayBitmap( BYTE * pBuffer );

	int GetPointToConvert( BYTE * pBuffer, int *nAxle_x, int *nAxle_y, int id_led);   

	void ConvertToRectifyImage(BYTE * pBuffer);  
    void ResetSmoothing();
	void smoothing(float warpedX, float warpedY);  
	void Exponentialsmoothing(float warpedX, float warpedY);  
	void showCalibration(int x, int y, int size);
	void ResetRectify();

	void getUnwantedPoint( BYTE * pBuffer, long buferSize );
	void subUnWantedPoint( BYTE * pBuffer, long bufferSize );
};


CSampleGrabberCB* SampleGrabberFun(int id);

#endif
