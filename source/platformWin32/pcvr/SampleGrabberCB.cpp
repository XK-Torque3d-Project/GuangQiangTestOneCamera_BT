//#define CHECK_CAMERA_ZHENLV 1

#include "SampleGrabberCB.h"
#include "PlayCapRectify.h"

#include "console/console.h"

#define CheckPointer(p,ret) {if((p)==NULL) return (ret);}

//int balance = 0;

int GrayThreshold = 60;
CSampleGrabberCB::CSampleGrabberCB(UINT camid)
{
	_ID = camid;
	Width = 0;
	Height = 0;
	lClientWidth = GetSystemMetrics(SM_CXSCREEN);
	lClientHeight = GetSystemMetrics(SM_CYSCREEN);
	m_nBrightDotCount = 0;
	//m_PNumber = 0;
	m_curMousePoint.X = -1;
	m_curMousePoint.Y = -1;

	m_mode = MODE_SET_CALIBRATION;
	//m_calibrationState = CALIBRATION_STATE::STATE_FIRST;
	m_bSwitch = false;
	m_lTickCount = 0;
	m_lLastFrameNumber = 0;
	m_lFps = 0;
	m_ncount = 0;
	m_nPointToConvert = 0;

	m_bCurPointModified = FALSE;
	//m_nGrayThreshold = 240;
	m_nMoveRadius = 0;
	ZeroMemory( m_p4, sizeof( m_p4 ) );

	ResetRectify();
	ResetSmoothing();
	InitRectifyCfg();

	m_nFirstInst = 0;
	b_getUnwantedLightSource = false;
	ZeroMemory( unwantedPoint, sizeof( unwantedPoint ) );
	unwantedPointNum = 0;
	getFrameNum = 0;
	m_funPointProc = NULL;
}

void CSampleGrabberCB:: InitRectifyCfg()
{
	WCHAR szwcKey[ 50 ], strTitle[50];
	CvPoint2D32f cvsrc[4];
	BOOL bRet = false;
	RECT rc;
	ZeroMemory(szwcKey, sizeof(szwcKey));
	ZeroMemory(strTitle, sizeof(strTitle));
	for( int i = 0; i < 4; i++ )
	{
		swprintf(szwcKey, sizeof( szwcKey ), L"DataSrc%d", i);
		swprintf(strTitle, sizeof(strTitle), L"Camera%d", (int)_ID);
		bRet = GetPrivateProfileStruct(strTitle, szwcKey, ( LPVOID )&cvsrc[ i ], sizeof( CvPoint2D32f ), L".//Rectangle.vro" );
		if( !bRet )
		{
			m_p4[ 0 ].X = 20;
			m_p4[ 0 ].Y = 20;
			m_p4[ 1 ].X = 60;
			m_p4[ 1 ].Y = 20;
			m_p4[ 2 ].X = 60;
			m_p4[ 2 ].Y = 60;
			m_p4[ 3 ].X = 20;
			m_p4[ 3 ].Y = 60;
			break;
		}

		GetWindowRect( GetDesktopWindow(), &rc );
		if( m_p4[ i ].X > rc.right - rc.left || m_p4[ i ].Y > rc.bottom - rc.top)
		{
			m_p4[ 0 ].X = 20;
			m_p4[ 0 ].Y = 20;
			m_p4[ 1 ].X = 60;
			m_p4[ 1 ].Y = 20;
			m_p4[ 2 ].X = 60;
			m_p4[ 2 ].Y = 60;
			m_p4[ 3 ].X = 20;
			m_p4[ 3 ].Y = 60;
			break;
		}
		else
		{
			m_p4[ i ].X = cvsrc[ i ].x;
			m_p4[ i ].Y = cvsrc[ i ].y;
		}
	}

	GrayThreshold = GetPrivateProfileInt(L"CameraInfo", L"GrayThreshold", 0, L".//cfg.ini");
	if (GrayThreshold <= 0 || GrayThreshold >= 255)
	{
		GrayThreshold = 125;
		WritePrivateProfileString(L"CameraInfo", L"GrayThreshold", L"125", L".//cfg.ini");
	}
	//m_nMoveRadius = GetPrivateProfileInt(strTitle, L"MoveRadius", m_nMoveRadius, L".//cfg.ini" );

	m_bYellowCon = false;
	m_nYellowIndex = -1;
	m_bConform = false;
	m_translate = cvCreateMat(3,3,CV_32FC1);
}

void CSampleGrabberCB::ResetRectify()
{
	m_ntempled = 999;
	g_bBeginDrawRectangle = false;
	m_bRectifyState = 0;
	m_nLed = -1;
	m_nPointToConvert = 0;
	m_ncount = 0;
	g_bled = false;
}

void CSampleGrabberCB::getUnwantedPoint( BYTE *pBuffer, long BufferSize )
{
	float fGray = 0.0f;
	unwantedPointNum = 0;
	for( int y = 0; y < Height; y++ )
	{
		for( int x = 0; x < Width * 3; x += 3 )
		{
			//Gray = (R*299 + G*587 + B*114 + 500) / 1000; //整数运算效率高于浮点运算.
			/*fGray = ( float )( 299 * pBuffer[ x + 2 + Width * 3 * y ] + 
					587 * pBuffer[ x + 1 + Width * 3 * y ] +				
					114 * pBuffer[ x + 0 + Width * 3 * y ] ) / 1000.0;*/
			//Gray = (R*19595 + G*38469 + B*7472) >> 16; //移位法效率更高.
			fGray = (float)((pBuffer[ x + 2 + Width * 3 * y ] * 19595
											+ pBuffer[ x + 1 + Width * 3 * y ] * 38469
											+ pBuffer[ x + 0 + Width * 3 * y ] * 7472) >> 16);	

			if( fGray > /*m_nGrayThreshold*/200 ) 
			{									
				unwantedPoint[unwantedPointNum].X = x;
				unwantedPoint[unwantedPointNum].Y = y;
				unwantedPointNum++;
			}
		}
	}
}

void CSampleGrabberCB::subUnWantedPoint( BYTE *pBuffer, long BuferSize )
{
	if ( unwantedPointNum == 0 )
	{
		return;
	}
	for ( int index = 0; index < unwantedPointNum; index++ )
	{
		int x = unwantedPoint[ index ].X;
		int y = unwantedPoint[ index ].Y;
		if ( ( x + 2 + Width * y * 3 ) >= BuferSize )
		{
			return;
		}
		pBuffer[ x + 2 + Width * y * 3 ] = 0;
	    pBuffer[ x + 1 + Width * y * 3 ] = 0;
		pBuffer[ x + 0 + Width * y * 3 ] = 0;
	}
}


#if CHECK_CAMERA_ZHENLV
double LastTimeVal = 0;
#endif
STDMETHODIMP CSampleGrabberCB::BufferCB(double SampleTime, BYTE *pBuffer, long BuferSize)
{
#if CHECK_CAMERA_ZHENLV
	//检测采集器的刷新帧率信息.
	double dTime = SampleTime - LastTimeVal;
	LastTimeVal = SampleTime;
	int camZhenLv = (int)(1 / dTime);
	Con::printf("dTime %.6f, camZhenLv %.6f", dTime, camZhenLv);
#endif

	POINT point = {0,0};

	/*if ( _ID == 1 )
	{
		balance++;
	}
	else if ( _ID == 2 )
	{
		balance--;
	}*/
	getFrameNum++;
	if ( /*b_getUnwantedLightSource*/ getFrameNum == 9000 )
	{
		getUnwantedPoint( pBuffer, BuferSize );
		getFrameNum = 0;
		return 0;
		//b_getUnwantedLightSource = false;
	}

	if (unwantedPointNum > 0)
	{
		subUnWantedPoint( pBuffer, BuferSize );
	}

	switch( m_mode )
	{
	case MODE_SET_CALIBRATION:
		if(g_bBeginDrawRectangle)
		{
			if(1 == m_bRectifyState)
			{	 
				int ax = 0, ay = 0, nled = -10;
				nled = m_nLed;
				GetPointToConvert(pBuffer, &ax, &ay, m_nLed);
				//Con::printf("sssssssssssssssssssssssssssssssssssssssssssssssssssss int   nled   m_nled   %d   %d",nled, m_nLed);

				m_nPointToConvert++;

				//find point
				if(nled != m_nLed)
				{
					if(m_nLed > 3)
					{
						m_bRectifyState = 0;
						Con::executef("onChangeCalibration",  Con::getIntArg(m_nLed));
					}
					else
					{
						m_p4[3 - m_nLed].X = ax;
						m_p4[3 - m_nLed].Y = ay;
						m_bRectifyState = 0;
						Con::executef("onChangeCalibration",  Con::getIntArg(m_nLed));
					}
				}
			}

			if(-1 == m_nLed)
			{ 
				//showCalibration(20, 20, 30);
			}
			else if(0 == m_nLed)
			{
				//showCalibration(lClientWidth - 24, 20, 30);
			}
			else if(1 == m_nLed)
			{
				//showCalibration(lClientWidth - 24, lClientHeight - 20, 30);
			}
			else if(2 == m_nLed)
			{
				//showCalibration(20, lClientHeight - 20, 30);
			}
			else if(3 == m_nLed)
			{
				if(_ID == 2)
				{
					g_bled = true;
				}
				else if(_ID == 1)
				{
					g_bled = true; 
					g_bBeginDrawRectangle = false;
				}

			}
		}

		DisplayRectifyImage(pBuffer, BuferSize);
		m_nFirstInst = 1;
		break;
	case MODE_MOTION:
		if(m_nFirstInst == 0)
		{
            DisplayRectifyImage(pBuffer, BuferSize);
			m_nFirstInst = 1;
		}
		Convert2GrayBitmap(pBuffer);

		if(m_bCurPointModified)
		{
			point.x = m_curMousePoint.X;
			point.y = m_curMousePoint.Y;
			m_bCurPointModified = FALSE;

			if(m_funPointProc)
			{
				if(point.x < 1 || point.y < 1)
				{
					point.x = 0;
					point.y = 0;
				}
				m_funPointProc(this->_ID, point);
				//MessageBox(NULL, L"m_funPointProcid", L"OK", MB_OK);
			}
		}
		else
		{
			if(m_funPointProc)
			{
				point.x = -1;
				point.y = -1;
				m_funPointProc(this->_ID, point);
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

STDMETHODIMP CSampleGrabberCB::SampleCB( double SampleTime, IMediaSample * pSample )
{
	return 0;
}

STDMETHODIMP CSampleGrabberCB::QueryInterface(REFIID riid, void ** ppv)
{
	CheckPointer(ppv,E_POINTER);
	if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown ) 
	{
		*ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
		return NOERROR;
	}
	return E_NOINTERFACE;
}

//void CSampleGrabberCB::DrawQuadrilateral(IN Point* fourPoint, IN INT count, Color &color, BYTE * pTargetBuffer )
//{
//	if( !fourPoint )
//		return;
//
//	if( count != 4 )
//		return;
//
//	Point points[ 4 ];
//
//	memcpy( points, fourPoint, sizeof( Point ) * 4 );
//
//	Bitmap bitmap( Width, Height, 3 * Width, PixelFormat24bppRGB, pTargetBuffer);
//
//	Graphics g(&bitmap);
//
//	SolidBrush brush( color );
//	Pen pen( &brush );
//
//	for( int i = 0; i < count; i++ )
//	{
//		points[ i ].X = points[ i ].X * ( ( float )Width / ( float )( lClientWidth ) );
//		points[ i ].Y = points[ i ].Y * ( ( float )Height / ( float )( lClientHeight ) );
//	}
//
//	g.DrawPolygon( &pen, points, 4 );
//
//	for( int i = 0; i < count; i++ )
//	{
//		const int len = 10;
//		// 鼠标单击移动时变色
//		if( m_bYellowCon && m_nYellowIndex == i )
//		{
//			brush.SetColor( Color( 50, 255, 255, 0 ) );
//		}
//		else
//		{
//			brush.SetColor( Color( 50, 255, 0, 0 ) );
//		}
//		g.FillRectangle( &brush, Rect( points[ i ].X - len, points[ i ].Y - len, 20, 20 ) );
//
//		WCHAR szNum[ 10 ];
//		ZeroMemory( szNum, sizeof( szNum ) );
//
//		swprintf_s( szNum, L"%d", i );
//		Font font( L"Times New Roman", 10 );
//
//		SolidBrush brush( Color( 255, 0, 0, 255 ) );
//
//		g.DrawString( szNum, wcslen( szNum ), &font, PointF( points[ i ].X - len, points[ i ].Y - len ), &brush );
//	}
//}

void CSampleGrabberCB::DisplayRectifyImage(BYTE * pBuffer, long BuferSize)
{
	CvPoint2D32f cvsrc[4];
	CvPoint2D32f cvdst[4];

	cvdst[ 0 ].x = 0;
	cvdst[ 0 ].y = 0;
	cvdst[ 1 ].x = lClientWidth;
	cvdst[ 1 ].y = 0;
	cvdst[ 2 ].x = lClientWidth;
	cvdst[ 2 ].y = lClientHeight;
	cvdst[ 3 ].x = 0;
	cvdst[ 3 ].y = lClientHeight;

	Point points[ 4 ];
	memcpy( points, m_p4, sizeof( Point ) * 4 );

	for( int i = 0; i < 4; i++ )
	{
		cvsrc[ i ].x = m_p4[ i ].X;
		cvsrc[ i ].y = m_p4[ i ].Y;

		cvdst[ i ].x = cvdst[ i ].x * ( ( float )Width / lClientWidth); 
		cvdst[ i ].y = cvdst[ i ].y * ( ( float )Height / lClientHeight);

		if( m_mode == MODE_SET_CALIBRATION )
		{
			WCHAR szwcKey[ 50 ], strTitle[50];
			swprintf( szwcKey, sizeof( szwcKey ), L"DataSrc%d", i );
			swprintf(strTitle, sizeof(strTitle), L"Camera%d", _ID);
			WritePrivateProfileStruct(strTitle, szwcKey, &cvsrc[ i ], sizeof( CvPoint2D32f ), L".//Rectangle.vro" );
			swprintf( szwcKey, sizeof( szwcKey ), L"DataDst%d", i );
			WritePrivateProfileStruct(strTitle, szwcKey, &cvdst[ i ], sizeof( CvPoint2D32f ), L".//Rectangle.vro" );
		}
	}

	m_warp.setSource( cvsrc[ 0 ].x, cvsrc[ 0 ].y, 
		cvsrc[ 1 ].x, cvsrc[ 1 ].y,
		cvsrc[ 2 ].x, cvsrc[ 2 ].y, 
		cvsrc[ 3 ].x, cvsrc[ 3 ].y);

	m_warp.setDestination( cvdst[ 0 ].x, cvdst[ 0 ].y, 
		cvdst[ 1 ].x, cvdst[ 1 ].y,
		cvdst[ 2 ].x, cvdst[ 2 ].y, 
		cvdst[ 3 ].x, cvdst[ 3 ].y);
}

void CSampleGrabberCB::Convert2GrayBitmap( BYTE * pBuffer )
{
	int nMax_x = 0;
	int nMax_y = 0;
	float nMaxx1 = 0.0f;
	float nMaxy1 = 0.0f;
	float fGray = 0.0f;

	float ax = 0.0f;
	float b = 0.0f;
	float ay = 0.0f;
	float X = 0.0f;
	float Y = 0.0f;
	bool bIsMouseInClient = false;
	m_nBrightDotCount = 0;

	for( int y = 0; y < Height; y++ )
	{
		for( int x = 0; x < Width * 3; x += 3 )
		{
			//Gray = (R*299 + G*587 + B*114 + 500) / 1000; //整数运算效率高于浮点运算.
			/*fGray = ( float )( 299 * pBuffer[ x + 2 + Width * 3 * y ] + 
					587 * pBuffer[ x + 1 + Width * 3 * y ] +				
					114 * pBuffer[ x + 0 + Width * 3 * y ] ) / 1000.0;*/
			//Gray = (R*19595 + G*38469 + B*7472) >> 16; //移位法效率更高.
			fGray = (float)((pBuffer[ x + 2 + Width * 3 * y ] * 19595
											+ pBuffer[ x + 1 + Width * 3 * y ] * 38469
											+ pBuffer[ x + 0 + Width * 3 * y ] * 7472) >> 16);

			if( fGray > GrayThreshold ) 
			{									
				fGray = 255;			
				m_nBrightDotCount++;	
				bIsMouseInClient = true;
			}
			else
			{
				fGray = 0;
			}
			(image->imageData + image->widthStep * y)[ x / 3] = fGray;
		}
	}

	for( int j = 0; j < image->height; j++ )
	{
		for( int i = 0; i < image->widthStep; i++ )
		{
			if( ( BYTE )((image->imageData + image->widthStep * j)[i] ) > 0 )
			{
				ax += ( BYTE )( (image->imageData + image->widthStep * j)[i] ) * ( i );
				ay += ( BYTE )( (image->imageData + image->widthStep * j)[i] ) * ( j );
				b += ( BYTE )( (image->imageData + image->widthStep * j)[i] );
			}
		}
	}

	if( b != 0 )
	{
		X = ax / b;
		Y = ay / b;
	}
	nMaxx1 = X;
	nMaxy1 = Y;

	float nx = 0.0f;
	float ny = 0.0f;
	m_warp.warp(nMaxx1, nMaxy1, nx, ny);

	nMaxx1 = nx;
	nMaxy1 = ny;
	Exponentialsmoothing(nMaxx1, nMaxy1);

	if(m_nBrightDotCount > 0)
	{
		RECT rc;
		HWND hWnd = GetDesktopWindow();
		GetWindowRect(hWnd, &rc );

		nMax_x = ( ( ( float )abs( rc.right - rc.left ) / (float)Width ) * nMaxx1 );
		nMax_y = ( ( ( float )abs( rc.bottom - rc.top ) / (float)Height ) * nMaxy1 );

		int d1 =  (int)abs(m_curMousePoint.X - (int)nMax_x );
		if(d1 > m_nMoveRadius)
		{
		    m_curMousePoint.X = nMax_x;
			m_bCurPointModified = TRUE;
		}

		int d2 = (int)abs(m_curMousePoint.Y  - (int)nMax_y);
		if(d2 > m_nMoveRadius)
		{
			m_curMousePoint.Y = nMax_y;
			m_bCurPointModified = TRUE;
		}
	}

	if( !bIsMouseInClient )
	{
		m_curMousePoint.X = -1;
		m_curMousePoint.Y = -1;
	}

	//m_PNumber = m_nBrightDotCount;
	//m_nBrightDotCount = 0;
}

STDMETHODIMP_(ULONG) CSampleGrabberCB::Release()
{
	cvReleaseImage(&image);

	if(m_translate)
	{
		cvReleaseMat( &m_translate );
		m_translate = NULL;
	}
	return 1;
}

int CSampleGrabberCB::GetPointToConvert( BYTE * pBuffer, int *nAxle_x, int *nAxle_y, int id_led)
{
	int nx = 0;
	int ny = 0;
	float fGray = 0.0f;
	//float ax = 0.0f;
	//float b = 0.0f;
	//float ay = 0.0f;
    
	bool bIsMouseInClient = false;
	m_nBrightDotCount = 0;
	for( int y = 0; y < Height; y++ )
	{
		for( int x = 0; x < Width * 3; x += 3 )
		{
			//Gray = (R*299 + G*587 + B*114 + 500) / 1000; //整数运算效率高于浮点运算.
			/*fGray = ( float )( 299 * pBuffer[ x + 2 + Width * 3 * y ] + 
					587 * pBuffer[ x + 1 + Width * 3 * y ] +				
					114 * pBuffer[ x + 0 + Width * 3 * y ] ) / 1000.0;*/
			//Gray = (R*19595 + G*38469 + B*7472) >> 16; //移位法效率更高.
			fGray = (float)((pBuffer[ x + 2 + Width * 3 * y ] * 19595
											+ pBuffer[ x + 1 + Width * 3 * y ] * 38469
											+ pBuffer[ x + 0 + Width * 3 * y ] * 7472) >> 16);
			
			if( fGray >  GrayThreshold) 
			{
				nx = x / 3;
				ny = y;
				bIsMouseInClient = true;
				m_nBrightDotCount++;	
				break;
			}
		}
	}

	if(bIsMouseInClient)
	{
		*nAxle_x = nx;
		*nAxle_y = ny;
		m_nLed++;
		m_ncount++;
	}
    return 1;
}

 //void CSampleGrabberCB::ConvertToRectifyImage( BYTE * pBuffer)
 //{
	// int nx = 0, ny = 0;
	// CvPoint2D32f cvsrc[4];
	// CvPoint2D32f cvdst[4];

	// cvdst[ 0 ].x = 0;
	// cvdst[ 0 ].y = 0;
	// cvdst[ 1 ].x = lClientWidth;
	// cvdst[ 1 ].y = 0;
	// cvdst[ 2 ].x = lClientWidth;
	// cvdst[ 2 ].y = lClientHeight;
	// cvdst[ 3 ].x = 0;
	// cvdst[ 3 ].y = lClientHeight;

	// for(int i = 0; i < 4; i++)
	// {
	//	 cvsrc[ i ].x = nx;
	//	 cvsrc[ i ].y = ny;

	//	 cvdst[ i ].x = cvdst[ i ].x * ( ( float )Width / ( float )( lClientWidth ) ); //屏幕的宽度 在摄像机的坐标
	//	 cvdst[ i ].y = cvdst[ i ].y * ( ( float )Height / ( float )( lClientHeight ) );
	// }

	// m_warp.setSource( cvsrc[ 0 ].x, cvsrc[ 0 ].y, 
	//	 cvsrc[ 1 ].x, cvsrc[ 1 ].y,
	//	 cvsrc[ 2 ].x, cvsrc[ 2 ].y, 
	//	 cvsrc[ 3 ].x, cvsrc[ 3 ].y);

	// m_warp.setDestination( cvdst[ 0 ].x, cvdst[ 0 ].y, 
	//	 cvdst[ 1 ].x, cvdst[ 1 ].y,
	//	 cvdst[ 2 ].x, cvdst[ 2 ].y, 
	//	 cvdst[ 3 ].x, cvdst[ 3 ].y);
 //}

 void CSampleGrabberCB::ResetSmoothing() 
 {
	 m_bSmoothState = false;
	 m_nSmoothingCount = 0;

	 //m_nSmoothPoints = 7;
	 //for( int i=0;i<10;i++)
	 //{
		// m_fSmoothingX[i] = 0;
		// m_fSmoothingY[i] = 0;
	 //}
	 //// 读取配置文件在C:\Windows\cfg.ini
	 //swprintf(strTitle, sizeof(strTitle), L"Camera%d", _ID);
	 //m_nSmoothPoints = GetPrivateProfileInt(strTitle, L"smoothpoints", m_nSmoothPoints, L"cfg.ini");

	 m_fMark = 0.05f;
	 m_fExsmothX = 0.0;
	 m_fExsmothY = 0.0;
 }

  /*void CSampleGrabberCB::smoothing(float warpedX, float warpedY)
  {
	  float sumX = warpedX, sumY = warpedY; 
	  float smoothWarpedX, smoothWarpedY; 
	  m_nSmoothingCount += 1; 

	  if (m_nSmoothingCount > m_nSmoothPoints)
	  {
		  m_nSmoothingCount = m_nSmoothPoints; 
	  }

	  for (int i = 0; i < m_nSmoothPoints - 1; i++)
	  {
		  m_fSmoothingX[i] = m_fSmoothingX[i + 1]; 
		  m_fSmoothingY[i] = m_fSmoothingY[i + 1];

		  sumX += m_fSmoothingX[i]; 
		  sumY += m_fSmoothingY[i];
	  }

	  m_fSmoothingX[m_nSmoothPoints - 1] = warpedX; 
	  m_fSmoothingY[m_nSmoothPoints - 1] = warpedY;

	  smoothWarpedX = sumX / (m_nSmoothingCount); 
	  smoothWarpedY = sumY / (m_nSmoothingCount);

	  warpedX = smoothWarpedX;
	  warpedY = smoothWarpedY;
  }*/

  void CSampleGrabberCB::Exponentialsmoothing(float warpedX, float warpedY)
  {
	  if(m_nSmoothingCount == 0)
	  {
          m_fExsmothX = warpedX;
		  m_fExsmothY = warpedY;
		  m_nSmoothingCount = 1;
	  }
	  else
	  {
          m_fExsmothX = m_fMark * warpedX + (1 - m_fMark) * m_fExsmothX;
		  m_fExsmothY = m_fMark * warpedY + (1 - m_fMark) * m_fExsmothY;
	  }

      warpedX = m_fExsmothX;
      warpedY = m_fExsmothY;
  }

  //void CSampleGrabberCB::showCalibration(int x, int y, int size)
  //{
	 // WCHAR szStr[500];
	 // Graphics g(g_hCalibrationWnd);
	 // Font font( L"Times New Roman", 15);
	 // SolidBrush brush1(Color( 255, 0, 0, 255 ));

	 // if(-1 == m_nLed)
	 // {
		//   if(m_ntempled != m_nLed)
		//   {
		//	  Bitmap bitmap0(L"zuoshang.bmp", FALSE);
		//	  g.DrawImage(&bitmap0, RectF( 0, 0, lClientWidth, lClientHeight));
		//	  Bitmap bitmap1(L"target1.bmp", FALSE);
		//	  g.DrawImage(&bitmap1, RectF( 0, 0, 50, 46));

		//	  ZeroMemory(szStr, sizeof(szStr));

		//	  if(_ID == 1)
		//	  {
		//		  swprintf_s(szStr, L"如图所示: 请用 1 枪瞄准画面左上角的靶心,然后扣动扳机");
		//		  /*swprintf(szStr, L"As the picture show: use the first gun to set the aim to left top on the picture , then Pulling the trigger!");*/
		//		  //swprintf(szStr, L"如图所示: 请用1枪瞄准画面左上角的靶心,然后扣动扳机 m_nLightcount = %d\t m_ncount = %d\t m_nPointToConvert = %d \t\n m_nLed = %d \ttempled = %d", m_nLightcount, m_ncount, m_nPointToConvert, m_nLed, m_ntempled);
		//	  }
		//	  else if(_ID == 2)
		//	  {
		//		  swprintf_s(szStr, L"如图所示: 请用 2 枪瞄准画面左上角的靶心,然后扣动扳机", NULL);
		//		 /* swprintf(szStr, L"As the picture show: use the second gun to set the aim to left top on the picture , then Pulling the trigger!");*/
		//	  }

		//	  g.DrawString(szStr, wcslen(szStr), &font, PointF(133, 66), &brush1);
		//   }
		//   m_ntempled = m_nLed;
	 // }
	 // else if(0 == m_nLed)
	 // {
		//  if(m_ntempled != m_nLed)
		//  {
		//	  Bitmap bitmap2(L"youshang.bmp", FALSE);
		//	  g.DrawImage(&bitmap2, RectF( 0, 0, lClientWidth, lClientHeight));
		//	  Bitmap bitmap3(L"target2.bmp", FALSE);
		//	  g.DrawImage(&bitmap3, RectF( lClientWidth - 50, 0, 50, 46));

		//	  ZeroMemory(szStr, sizeof(szStr));
		//	  swprintf_s(szStr, L"如图所示: 请瞄准画面右上角的靶心,然后扣动扳机" );
		//	  /*swprintf(szStr, L"As the picture show: use the gun to set the aim to right top on the picture , then Pulling the trigger!");*/
		//	  //swprintf(szStr, L"如图所示: 请瞄准画面右上角的靶心,然后扣动扳机 m_nLightcount = %d \t m_ncount = %d\t m_nPointToConvert = %d \t\n m_nLed = %d \t m_ntempled = %d", m_nLightcount, m_ncount, m_nPointToConvert, m_nLed, m_ntempled);

		//	  g.DrawString(szStr, wcslen(szStr), &font, PointF(133, 66), &brush1);
		//  }
		//  m_ntempled = m_nLed;
	 // }
	 // else if (1 == m_nLed)
	 // {
		//   if(m_ntempled != m_nLed)
		//   {
		//	  Bitmap bitmap4(L"youxia.bmp", FALSE);
		//	  g.DrawImage(&bitmap4, RectF( 0, 0, lClientWidth, lClientHeight));
		//	  Bitmap bitmap5(L"target3.bmp", FALSE);
		//	  g.DrawImage(&bitmap5, RectF( lClientWidth - 50, lClientHeight - 46, 50, 46));

		//	  ZeroMemory(szStr, sizeof(szStr));
		//	  swprintf_s(szStr, L"如图所示: 请瞄准画面右下角的靶心,然后扣动扳机");
		//	  /*swprintf(szStr, L"As the picture show: use the gun to set the aim to right bottom on the picture , then Pulling the trigger!");*/
		//	  //swprintf(szStr, L"如图所示: 请瞄准画面右下角的靶心,然后扣动扳机 m_nLightcount = %d \t m_ncount = %d \t m_nPointToConvert = %d \t\n m_nLed = %d \t m_ntempled = %d", m_nLightcount, m_ncount, m_nPointToConvert, m_nLed, m_ntempled);

		//	  g.DrawString(szStr, wcslen(szStr), &font, PointF(133, 66), &brush1);
		//   }
		//   m_ntempled = m_nLed;
	 // }
	 // else if (2 == m_nLed)
	 // {
		//  if(m_ntempled != m_nLed)
		//  {
		//	  Bitmap bitmap6(L"zuoxia.bmp", FALSE);
		//	  g.DrawImage(&bitmap6, RectF( 0, 0, lClientWidth, lClientHeight));
		//	  Bitmap bitmap7(L"target4.bmp", FALSE);
		//	  g.DrawImage(&bitmap7, RectF( 0, lClientHeight - 46, 50, 46));

		//	  ZeroMemory(szStr, sizeof(szStr));
		//	  swprintf_s(szStr, L"如图所示: 请瞄准画面左下角的靶心,然后扣动扳机 ");
		//	  /*swprintf(szStr, L"As the picture show: use the gun to set the aim to left bottom on the picture , then Pulling the trigger!");*/
		//	  //swprintf(szStr, L"如图所示: 请瞄准画面左下角的靶心,然后扣动扳机 m_nLightcount = %d \t m_ncount = %d\t m_nPointToConvert = %d \t m_nLed = %d", m_nLightcount, m_ncount, m_nPointToConvert, m_nLed);

		//	  g.DrawString(szStr, wcslen(szStr), &font, PointF(133, 66), &brush1);
		//   }
		//   m_ntempled = m_nLed;
	 // }
  //}