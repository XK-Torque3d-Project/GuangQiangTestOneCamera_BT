
#include "pcvr.h"

//////////////////////////////////////
#include "T3D/shapeBase.h"
#include "math/mRandom.h"
#include "platform/platform.h"
#include <time.h>
#include <stdio.h>

#include "PlayCapRectify.h"
#include "SampleGrabberCB.h"
#include "T3D/gameBase/gameConnection.h"

#pragma comment(lib, "SLABHIDDevice.lib")

pcvr* gPcvr = NULL;								// 硬件控制全局指针

#define	HID_Blinky_VID	0x10C4
#define	HID_Blinky_PID	0x8468

#ifndef HID_BUFFER_LEN
#define HID_BUFFER_LEN 63
#endif

#define SENDMSG_HEAD1	11	//获取信息字头一
#define SENDMSG_HEAD2	22	//获取信息字头二
#define SENDMSG_END1		33	//获取信息字尾一
#define SENDMSG_END2		44	//获取信息字尾二

void CALLBACK TimerProc(HWND hWnd,UINT nMsg,UINT nTimerid,DWORD dwTime); 


void CALLBACK PointProc1(int nID, POINT point)
{
	if (bGameTestSelf)
	{
		return;
	}

	HWND hDesktopWnd = GetDesktopWindow();
	RECT rc;

	GetWindowRect( hDesktopWnd, &rc );

	if((point.x > -1) && (point.y > -1))// 两个点都大于0
	{
		gPcvrPointS0.x = point.x;
		gPcvrPointS0.y = rc.bottom - point.y;
		gPcvrPointS0.z = 1.0f;
		//Con::printf("gpcvrpoints0   %f  %f  %f",gPcvrPointS0.x,gPcvrPointS0.y,gPcvrPointS0.z);
	}
	else if( point.x == -1 && point.y == -1 )
	{
		//光标出屏
	}
} 

void CALLBACK PointProc2(int nID, POINT point)
{
	if (bGameTestSelf)
	{
		return;
	}

	HWND hDesktopWnd = GetDesktopWindow();
	RECT rc;

	GetWindowRect( hDesktopWnd, &rc );

	if( ( point.x > -1) && (point.y > -1 ) )// 两个点都大于0
	{
		gPcvrPointS1.x = point.x;
		gPcvrPointS1.y = rc.bottom - point.y;
		gPcvrPointS1.z = 1.0f;
		//Con::printf("gpcvrpoints1   %f  %f  %f",gPcvrPointS1.x,gPcvrPointS1.y,gPcvrPointS1.z);
	}
	else if( point.x == -1 && point.y == -1 )
	{
		//光标出屏
	}
}

pcvr::pcvr()
{
	if ( !Init() )
	{
		Con::errorf("pcvr init is not success");
	}
}

pcvr::~pcvr(void)
{
	
}

bool pcvr::Init(void)
{
	ZeroMemory( mSendArray, sizeof( mSendArray ) );
	ZeroMemory( mGetArray, sizeof( mGetArray ) );
	ZeroMemory( mPlayerShotMsg, sizeof( mPlayerShotMsg ) );
	ZeroMemory( mPlayerStartMsg, sizeof( mPlayerStartMsg ) );
	ZeroMemory( mRocketShotMsg, sizeof( mRocketShotMsg ) );
	ZeroMemory( mPlayerCoinMsg, sizeof( mPlayerCoinMsg ) );
	ZeroMemory( mPlayerCoordMsg, sizeof( mPlayerCoordMsg ) );
	ZeroMemory( mPlayerShakeGunMsg, sizeof( mPlayerShakeGunMsg ) );
	ZeroMemory( mRockerRot, sizeof( mRockerRot ) );
	ZeroMemory( mSetPlaneMsg, sizeof( mRockerRot ) );
	ZeroMemory( mPedalAniMsg, sizeof( mPedalAniMsg ) );
	ZeroMemory( mOnSetBasePointState, sizeof(mOnSetBasePointState) );
	ZeroMemory( mSetBasePointKeyDown, sizeof(mSetBasePointKeyDown) );
	ZeroMemory( mPlayerOpenGunMsg, sizeof( mPlayerOpenGunMsg ) );

	srand(GetCurrentTime());

	mCameraEnable = false;
	mNeedRocker = false;
	mSendHidMsgToScripts = false;

	mPlayerCanSubCoin[0] = false;
	mPlayerCanSubCoin[1] = false;
	mPlayerSubCoinNum[0] = 0;
	mPlayerSubCoinNum[1] = 0;

	return true;
}

bool pcvr::createGun()
{
	if (HidDevice_IsOpened( gTestHidPtr ) )
	{
		HidDevice_Close(gTestHidPtr);
	}
	else
	{
		int number = HidDevice_GetNumHidDevices( HID_Blinky_VID, HID_Blinky_PID );
		BYTE result = HidDevice_Open( &gTestHidPtr, 0, HID_Blinky_VID, HID_Blinky_PID, 1 );
		if (result != HID_DEVICE_SUCCESS)
		{
			Con::executef( "OnConnectState", "2" );
			return false;
		}
		else
		{
			Con::executef( "OnConnectState", "1" );
			HidDevice_SetTimeouts( gTestHidPtr, 0, 1 );
			createCamera();
			return true;
		}
	}
	return false;
}

bool pcvr::deleteGun()
{
	HidDevice_FlushBuffers(gTestHidPtr);
	HidDevice_CancelIo(gTestHidPtr);
	HidDevice_Close(gTestHidPtr);
	releaseCamera();
	return true;
}

bool pcvr::openDevice()
{
	if ( HidDevice_IsOpened( gTestHidPtr ) )
	{
		HidDevice_Close(gTestHidPtr);
	}
	int number = HidDevice_GetNumHidDevices( HID_Blinky_VID, HID_Blinky_PID );
	BYTE result = HidDevice_Open( &gTestHidPtr, 0, HID_Blinky_VID, HID_Blinky_PID, 1 );
	if (result != HID_DEVICE_SUCCESS)
	{
		return false;
	}
	else
	{
		Con::executef( "OnConnectState", "1" );
		HidDevice_SetTimeouts( gTestHidPtr, 0, 1000 );
		return true;
	}
	return false;
}

void pcvr::keyProcess()
{
	//判断几个异或值是否正确
	//4----2 3 5 6 7
	//31---10-35
	//51---44-59
	BYTE valueT = 0x00;
	for (int l=2; l<8; l++)
	{
		if (l != 4)
		{
			valueT ^= mGetArray[l-2];
		}
	}

	if (mGetArray[2] != valueT)
	{
		Con::printf("mGetArray[2]   buffer[4]");
		return;
	}
	
	valueT = 0x00;

	for (int m=10; m<36; m++)
	{
		if (m != 31)
		{
			valueT ^= mGetArray[m-2];
		}
	}

	if (mGetArray[29] != valueT)
	{
		Con::printf("mGetArray[29]   buffer[31]");
		return;
	}

	valueT = 0x00;

	for (int n=44; n<60; n++)
	{
		if (n != 51)
		{
			valueT ^= mGetArray[n-2];
		}
	}

	if (mGetArray[49] != valueT)
	{
		Con::printf("mGetArray[49]   buffer[51]");
		return;
	}
	//Con::printf("get  %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d ", mGetArray[8],mGetArray[9],mGetArray[10],mGetArray[11],mGetArray[12],mGetArray[13],mGetArray[14],mGetArray[15],mGetArray[16],mGetArray[17],mGetArray[18],mGetArray[19],mGetArray[20],mGetArray[21],mGetArray[22],mGetArray[23]);
	//Con::errorf("geh buffer[8] %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x ", mGetArray[6],mGetArray[7],mGetArray[8],mGetArray[9],mGetArray[10],mGetArray[11],mGetArray[12],mGetArray[13],mGetArray[14],mGetArray[15],mGetArray[16],mGetArray[17],mGetArray[18],mGetArray[19],mGetArray[20],mGetArray[21],mGetArray[22],mGetArray[23]);
	//四个玩家按键信息
	for ( int i = 0; i < 2; i++ )
	{
		BYTE tempValue1 = 0x00;
		BYTE tempValue2 = 0x00;
		BYTE tempValue3 = 0x00;

		//开枪信息
		if (i == 0)
		{
			tempValue1 = 0xaa;
			tempValue2 = 0x55;
			tempValue3 = mGetArray[i+10];
		}
		else if (i == 1)
		{
			tempValue1 = 0x31;
			tempValue2 = 0x40;
			tempValue3 = mGetArray[i+10] & 0x71;
		}

		if ( tempValue3 == tempValue1 && mPlayerOpenGunMsg[i] )
		{
			if ( !mPlayerShotMsg[i] )
			{
				//Con::printf("player%d shot ", i );
				mPlayerShotMsg[i] = true;
				if ( i == 0 )
				{
					Con::executef( "playerOnePressShoot", "1" );
				}
				else if ( i == 1 )
				{
					Con::executef( "playerTwoPressShoot", "1" );
				}
				if ( mSendHidMsgToScripts )
				{
					char *retBuffer = Con::getReturnBuffer(10);
					dSprintf(retBuffer, 10, "%d", i );
					Con::executef("onChooseButtonDown", retBuffer );
				}
			}
		}
		else if ( tempValue3 == tempValue2 && mPlayerOpenGunMsg[i] )
		{
			if ( i == 0 )
			{
				Con::executef( "playerOnePressShoot", "0" );
			}
			else if ( i == 1 )
			{
				Con::executef( "playerTwoPressShoot", "0" );
			}
			if ( mSendHidMsgToScripts && mPlayerShotMsg[i] )
			{
				char *retBuffer = Con::getReturnBuffer(10);
				dSprintf(retBuffer, 10, "%d", i );
				Con::executef("onChooseButtonUp", retBuffer );
			}
			mPlayerShotMsg[i] = false;
		}

		//开始信息
		if (i == 0)
		{
			tempValue1 = 0x04;
			tempValue2 = 0x0b;
			tempValue3 = mGetArray[i+8] & 0x0f;
		}
		else if (i == 1)
		{
			tempValue1 = 0x82;
			tempValue2 = 0x60;
			tempValue3 = mGetArray[i+8] & 0xe2;
		}

		if ( tempValue3 == tempValue1 )
		{
			if ( !mPlayerStartMsg[i] )
			{
				mPlayerStartMsg[i] = true;
				if ( i == 0 )
				{
					Con::executef( "startPlayer1Game", "1" );
				}
				else if ( i == 1 )
				{
					Con::executef( "startPlayer2Game", "1" );
				}
				
			}
		}
		else if ( tempValue3 == tempValue2 )
		{
			if (i == 0 && mPlayerStartMsg[i])
			{
				Con::executef( "startPlayer1Game", "0" );
			}
			else if (i == 1 && mPlayerStartMsg[i])
			{
				Con::executef( "startPlayer2Game", "0" );
			}
			mPlayerStartMsg[i] = false;
		}

		//投币信息
		if ( mPlayerCoinMsg[i] != mGetArray[i+6] )
		{
			mPlayerCoinMsg[i] = mGetArray[i+6];
			Con::printf("Player%d coin:%d", i, mPlayerCoinMsg[i] );
			Con::errorf("Player total coin num:%d", mPlayerCoinMsg[0] +mPlayerCoinMsg[1] +mPlayerCoinMsg[2] +mPlayerCoinMsg[3] );
			Con::executef( "insertCoin", "1" );
		}
	}

	//摇杆信息
	//if need rocker
	if (mNeedRocker)
	{
		for ( int i = 0; i < 4; i++ )
		{
			BYTE tempValue1 = 0x00;
			BYTE tempValue2 = 0x00;
			BYTE tempValue3 = 0x00;

			if (i == 0)
			{
				tempValue1 = 0xaa;
				tempValue2 = 0x55;
				tempValue3 = mGetArray[i+14];
			}
			else if (i == 1)
			{
				tempValue1 = 0x19;
				tempValue2 = 0x84;
				tempValue3 = mGetArray[i+14] & 0x9d;
			}
			else if (i == 2)
			{
				tempValue1 = 0x04;
				tempValue2 = 0x0b;
				tempValue3 = mGetArray[i+14] & 0x0f;
			}
			else if (i == 3)
			{
				tempValue1 = 0x82;
				tempValue2 = 0x60;
				tempValue3 = mGetArray[i+14] & 0xe2;
			}

			if ( tempValue3 == tempValue1 )
			{
				//Con::printf("player rocker %d", i );
				mRockerRot[i] = true;
			}
			else if ( tempValue3 == tempValue2 )
			{
				if ( i == 0 )
				{
					Con::executef( "PDDunpaiMoveU", "0" );
				}
				else if ( i == 1 )
				{
					Con::executef( "PDDunpaiMoveD", "0" );
				}
				else if ( i == 2 )
				{
					Con::executef( "PDDunpaiMoveL", "0" );
				}
				else if ( i == 3 )
				{
					Con::executef( "PDDunpaiMoveR", "0" );
				}

				mRockerRot[i] = false;
			}
		}

		if (mRockerRot[0] && !mRockerRot[1] && !mRockerRot[2] && !mRockerRot[3])
		{
			Con::executef( "PDDunpaiMoveU", "1" );
		}
		else if (!mRockerRot[0] && mRockerRot[1] && !mRockerRot[2] && !mRockerRot[3])
		{
			Con::executef( "PDDunpaiMoveD", "1" );
		}
		else if (!mRockerRot[0] && !mRockerRot[1] && mRockerRot[2] && !mRockerRot[3])
		{
			Con::executef( "PDDunpaiMoveL", "1" );
		}
		else if (!mRockerRot[0] && !mRockerRot[1] && !mRockerRot[2] && mRockerRot[3])
		{
			Con::executef( "PDDunpaiMoveR", "1" );
		}
		else if (mRockerRot[0] && !mRockerRot[1] && mRockerRot[2] && !mRockerRot[3])
		{
			Con::executef( "PDDunpaiMoveLU", "1" );
		}
		else if (mRockerRot[0] && !mRockerRot[1] && !mRockerRot[2] && mRockerRot[3])
		{
			Con::executef( "PDDunpaiMoveRU", "1" );
		}
		else if (!mRockerRot[0] && mRockerRot[1] && mRockerRot[2] && !mRockerRot[3])
		{
			Con::executef( "PDDunpaiMoveLD", "1" );
		}
		else if (!mRockerRot[0] && mRockerRot[1] && !mRockerRot[2] && mRockerRot[3])
		{
			Con::executef( "PDDunpaiMoveRD", "1" );
		}
	}

	for ( int i = 0; i < 2; i++ )
	{
		BYTE tempValue1 = 0x00;
		BYTE tempValue2 = 0x00;
		BYTE tempValue3 = 0x00;

		//甩枪信息
		if (i == 0)
		{
			tempValue1 = 0x04;
			tempValue2 = 0x0b;
			tempValue3 = mGetArray[i+20] & 0x0f;
		}
		else if (i == 1)
		{
			tempValue1 = 0x82;
			tempValue2 = 0x60;
			tempValue3 = mGetArray[i+20] & 0xe2;
		}

		if ( tempValue3 == tempValue1 )
		{
			if ( !mPlayerShakeGunMsg[i] )
			{
				//Con::printf( "player%d shake gun", i );
				mPlayerShakeGunMsg[i] = true;
				if ( i == 0 )
				{
					Con::executef( "huidaoP1", "1" );
				}
				else if ( i == 1 )
				{
					Con::executef( "huidaoP2", "1" );
				}
			}
		}
		else if ( tempValue3 == tempValue2 )
		{
			mPlayerShakeGunMsg[i] = false;
		}

		//设置按键信息 0-set; 1-move
		if (i == 0)
		{
			tempValue1 = 0x42;
			tempValue2 = 0x24;
			tempValue3 = mGetArray[i+18] & 0x66;
		}
		else if (i == 1)
		{
			tempValue1 = 0x41;
			tempValue2 = 0x22;
			tempValue3 = mGetArray[i+18] & 0x63;
		}

		if ( tempValue3 == tempValue1 )
		{
			if ( !mSetPlaneMsg[i] )
			{
				mSetPlaneMsg[i] = true;
				Con::executef( "onIntoGameSetGui",  Con::getIntArg(i) );
				//Con::printf( "set button %d dow", i );
			}
		}
		else if ( tempValue3 == tempValue2 )
		{
			mSetPlaneMsg[i] = false;
		}

		//脚踏板信息
		if (i == 0)
		{
			tempValue1 = 0x18;
			tempValue2 = 0x06;
			tempValue3 = mGetArray[i+12] & 0x1e;
		}
		else if (i == 1)
		{
			tempValue1 = 0x0e;
			tempValue2 = 0x80;
			tempValue3 = mGetArray[i+12] & 0x8e;
		}

		if ( tempValue3 == tempValue1 )
		{
			if ( !mPedalAniMsg[i] )
			{
				mPedalAniMsg[i] = true;
				//Con::printf( "Pedal %d down", i );
				if ( i == 0)
				{
					Con::executef( "playerOneOnButtonDown", "0" );
				}
				else if ( i == 1 )
				{
					Con::executef( "playerTwoOnButtonDown", "0" );
				}
			}
		}
		else if ( tempValue3 == tempValue2 )
		{
			if (mPedalAniMsg[i] && i == 0)
			{
				Con::executef( "playerOneOnButtonDown", "1" );
			}
			else if (mPedalAniMsg[i] && i == 1 )
			{
				Con::executef( "playerTwoOnButtonDown", "1" );
			}

			mPedalAniMsg[i] = false;
		}
	}
	camSetBasePoint();
}

void pcvr::cleanSendMsg()
{
	ZeroMemory( mSendArray, sizeof( mSendArray ) );
}

void pcvr::printArray( BYTE * array, int arraySize )
{
	char buffer[1024];
	int j = 0;
	for( int i = 0; i < arraySize; i++ )
	{
		j += sprintf( buffer + j, " %d ", array[i] );
	}
	Con::printf( buffer );
}

void pcvr::setSendMsg( int index, BYTE val )
{
	if ( index < 0 || index > ( sizeof( mSendArray ) - 1 ) )
	{
		Con::errorf("invalid index %d for setSendMsg!", index );
	}
	else
	{
		mSendArray[ index ] = val;
	}
}

void pcvr::subPlayerCoin( int playerIndex, int coinNum )
{
	if ( playerIndex < 0 || playerIndex > 3 )
	{
		Con::errorf("invalid index %d for subPlayerCoin!", playerIndex );
	}
	else
	{
		int coinNumTemp = coinNum;
		if (mPlayerCoinMsg[0] >= coinNum)
		{
			mSendArray[0+5] = coinNum;
			mPlayerSubCoinNum[0] = coinNum;
			mPlayerCanSubCoin[0] = true;
			return;
		}
		else
		{
			mSendArray[0+5] = mPlayerCoinMsg[0];
			//coinNumTemp -= mPlayerCoinMsg[0];
			mPlayerSubCoinNum[0] = mPlayerCoinMsg[0];
			mPlayerCanSubCoin[0] = true;
			coinNumTemp -= mPlayerCoinMsg[0];
		}

		if (mPlayerCoinMsg[1] >= coinNumTemp)
		{
			mSendArray[1+5] = coinNumTemp;
			mPlayerSubCoinNum[1] = coinNumTemp;
			mPlayerCanSubCoin[1] = true;
			return;
		}
		else
		{
			mSendArray[1+5] = mPlayerCoinMsg[1];
			mPlayerSubCoinNum[1] = mPlayerCoinMsg[1];
			mPlayerCanSubCoin[1] = true;
			//coinNumTemp -= mPlayerCoinMsg[1];
		}

		/*if (mPlayerCoinMsg[2] >= coinNumTemp)
		{
			mSendArray[2+2] = coinNumTemp;
			return;
		}
		else
		{
			mSendArray[2+2] = mPlayerCoinMsg[2];
			coinNumTemp -= mPlayerCoinMsg[2];
		}

		if (mPlayerCoinMsg[3] >= coinNumTemp)
		{
			mSendArray[3+2] = coinNumTemp;
			return;
		}
		else
		{
			mSendArray[3+2] = mPlayerCoinMsg[3];
			coinNumTemp -= mPlayerCoinMsg[3];
		}*/
	}
}

S32 pcvr::getCoinNum()
{
	//return mPlayerCoinMsg[0] + mPlayerCoinMsg[1] + mPlayerCoinMsg[2] + mPlayerCoinMsg[3];
	return mPlayerCoinMsg[0] + mPlayerCoinMsg[1];
}

void pcvr::flashPlayerStartLight( int playerIndex, bool flash )
{
	if ( playerIndex < 0 || playerIndex > 3 )
	{
		Con::errorf("invalid index %d for flashPlayerStartLight!", playerIndex );
	}
	else
	{
		if (playerIndex == 0)
		{
			playerIndex = 22;
		}
		else if (playerIndex == 1)
		{
			playerIndex = 35;
		}

		if ( flash )
		{
			mSendArray[playerIndex] = 0xaa;
		}
		else
		{
			mSendArray[playerIndex] = 0x55;
		}
	}
}

void pcvr::setShakeLevel( int playerIndex, int val )
{
	if ( playerIndex < 0 || playerIndex > 3 )
	{
		Con::errorf("invalid index %d for setShakeLevel!", playerIndex );
	}
	else
	{
		if (playerIndex == 0)
		{
			playerIndex = playerIndex + 17;
		}
		else
		{
			playerIndex = playerIndex + 18;
		}

		if (val == 6)
		{
			mSendArray[playerIndex] = 0x19 - 9;
		}
		else
		{
			mSendArray[playerIndex] = 0x19 - 2 * (val - 1);
		}
	}
}

void pcvr::setBallonetState( int ballonetIndex, bool val )
{
	if ( ballonetIndex < 0 || ballonetIndex > 5 )
	{
		Con::errorf("invalid index %d for setBallonetState!", ballonetIndex );
	}
	else
	{
		if (ballonetIndex == 0 || ballonetIndex == 1)
		{
			ballonetIndex += 40;
		}
		else if (ballonetIndex == 2 || ballonetIndex == 3)
		{
			ballonetIndex += 34;
		}

		if ( val )
		{
			mSendArray[ballonetIndex] = 0xaa;
		}
		else
		{
			mSendArray[ballonetIndex] = 0x55;
		}
	}
}

void pcvr::setProcessLightLevel( int level )
{
	if ( level < 0 || level > 10 )
	{
		Con::errorf("wrong val %d set for setProcessLightLevel", level );
	}
	else
	{
		if (level < 10)
		{
			mSendArray[20] =( 0x10 + level);
		}
		else
		{
			mSendArray[20] =( 0x10 + 0x0a);
		}
	}
}

void pcvr::setGunShakeState( int playerIndex, int state )
{
	if ( playerIndex < 0 || playerIndex > 1 )
	{
		Con::errorf("invalid index %d for mSendArray!", playerIndex );
	}
	else
	{
		if ( state != 0 && state != 1 && state != 2 )
		{
			Con::warnf("wrong shake state %d set for setGunShakeState", state );
			return;
		}

		if (playerIndex == 0)
		{
			playerIndex = 16;
		}
		else if(playerIndex == 1)
		{
			playerIndex = 18;
		}

		//单发震动允许
		if ( state == 1 )
		{
			mSendArray[playerIndex] = 0xaa;
		}
		else if ( state == 2 ) //连发震动允许
		{
			mSendArray[playerIndex] = 0x55;
		}
		else //禁止震动
		{
			mSendArray[playerIndex] = 0x00;
		}

		//mSendArray[playerIndex+1] = 0x15;
	}
}

//camera
void pcvr::createCamera()
{
	if( RunGun() )
	{
		CSampleGrabberCB *CSgcb1 = SampleGrabberFun(ID_CAMERA1);
		CSampleGrabberCB *CSgcb2 = SampleGrabberFun(ID_CAMERA2);
		CSgcb2->m_mode = MODE::MODE_MOTION;
		CSgcb1->m_mode = MODE::MODE_MOTION;
		PlayVideo();
		mCameraEnable = true;
		setCamPointCallBackFun();
	}
	else
	{
		MessageBox( NULL, L"Can not find Camera!", L"error", MB_OK);
	}
}

void pcvr::reSetCamera()
{
	CSampleGrabberCB *CSgcb1 = NULL;
	CSgcb1 = SampleGrabberFun(ID_CAMERA1);
	if ( CSgcb1 != NULL )
	{
		CSgcb1->g_bled = true;
		CSgcb1->m_mode = MODE_MOTION;
		CSgcb1->g_bBeginDrawRectangle = false;
	}
	CSampleGrabberCB *CSgcb2 = NULL;
	CSgcb2 = SampleGrabberFun(ID_CAMERA2);
	if ( CSgcb2 != NULL )
	{
		CSgcb2->m_mode = MODE_MOTION;
		CSgcb2->g_bBeginDrawRectangle = false;
	}
}

void pcvr::releaseCamera()
{
	StopVideo();
	CloseGun();
}

void pcvr::setCamPointCallBackFun()
{
	CSampleGrabberCB *g_pCB = NULL;
	CSampleGrabberCB *g_pCB1 = NULL;

	g_pCB = SampleGrabberFun(ID_CAMERA1);
	g_pCB1 = SampleGrabberFun(ID_CAMERA2);

	g_pCB->m_funPointProc = PointProc1;
	g_pCB1->m_funPointProc = PointProc2;
}

void pcvr::setBasePoint( int playerIndex )
{
	if ( playerIndex == 0)
	{
		CSampleGrabberCB *g_pCB = NULL;
		g_pCB = SampleGrabberFun(ID_CAMERA1);
		if ( g_pCB == NULL )
		{
			return;
		}
		PlayVideo();
		g_pCB->ResetRectify();
		g_pCB->m_mode = MODE_SET_CALIBRATION;
		g_pCB->g_bBeginDrawRectangle = true;
		mOnSetBasePointState[0] = true;
	}
	if ( playerIndex == 1 )
	{
		CSampleGrabberCB *g_pCB1 = NULL;
		g_pCB1 = SampleGrabberFun(ID_CAMERA2);
		if ( g_pCB1 == NULL )
		{
			return;
		}
		PlayVideo();
		g_pCB1->ResetRectify();
		g_pCB1->m_mode = MODE_SET_CALIBRATION;
		g_pCB1->g_bBeginDrawRectangle = true;
		mOnSetBasePointState[1] = true;
	}
	openPlayerGun(playerIndex, true );
}

void pcvr::enableCamera( bool enable )
{
	if ( enable )
	{
		mCameraEnable = true;
		PlayVideo();
	}
	else
	{
		mCameraEnable = false;
		StopVideo();
	}
}

void pcvr::camSetBasePoint()
{
	//1p校正信息
	if ( mOnSetBasePointState[0] && mGetArray[ 10 ] == 0xaa && !mSetBasePointKeyDown[0] )
	{
		mSetBasePointKeyDown[0] = true;
		CSampleGrabberCB *CSgcb1 = SampleGrabberFun(ID_CAMERA1);
		if(0 == CSgcb1->m_bRectifyState)
			CSgcb1->m_bRectifyState = 1;
	}
	else if( mGetArray[ 10 ] == 0x55 )
	{
		mSetBasePointKeyDown[0] = false;
	}
	if ( mOnSetBasePointState[0] )
	{
		CSampleGrabberCB *CSgcb1 = NULL;
		CSgcb1 = SampleGrabberFun(ID_CAMERA1);
		if ( CSgcb1 == NULL )
		{
			return;
		}
		if(CSgcb1->g_bled )
		{
			CSgcb1->m_mode = MODE_MOTION;
			CSgcb1->g_bBeginDrawRectangle = false;
			mOnSetBasePointState[0] = false;
		}
	}

	//2p校正信息
	if ( mOnSetBasePointState[1] && mGetArray[11] == 0x31 && !mSetBasePointKeyDown[1] )
	{
		mSetBasePointKeyDown[1] = true;
		CSampleGrabberCB *CSgcb2 = SampleGrabberFun(ID_CAMERA2);
		if(0 == CSgcb2->m_bRectifyState)
			CSgcb2->m_bRectifyState = 1;
	}
	else if ( mGetArray[11] == 0x40 )
	{
		mSetBasePointKeyDown[1] = false;
	}
	if ( mOnSetBasePointState[1] )
	{
		CSampleGrabberCB *CSgcb2 = NULL;
		CSgcb2 = SampleGrabberFun(ID_CAMERA2);
		if ( CSgcb2 == NULL )
		{
			return;
		}
		if(CSgcb2->g_bled )
		{
			CSgcb2->m_mode = MODE_MOTION;
			CSgcb2->g_bBeginDrawRectangle = false;
			
			mOnSetBasePointState[1] = false;
		}
	}
}

void pcvr::openPlayerGun(int playerIndex, bool flag)
{
	BYTE val;
	if( flag )
	{
		val = 0xaa;
	}
	else
	{
		val = 0x55;
	}
	mSendArray[playerIndex + 43 ] = val;
	mPlayerOpenGunMsg[playerIndex] = flag;
}

void pcvr::sendMessage()
{
	if ( HidDevice_IsOpened( gTestHidPtr ) )
	{
		BYTE buffer[HID_BUFFER_LEN];
		ZeroMemory(buffer , HID_BUFFER_LEN);
		CopyMemory( buffer, mSendArray, sizeof( mSendArray ) );

		buffer[0] = 0x02;
		buffer[1] = 0x55;
		buffer[2] = 0x42;
		buffer[3] = 0x54;

		/*Con::printf("SendMessage:");
		printArray( buffer, sizeof( buffer ) );*/
		cleanSendMsg();

		setRandomValue(buffer);

		mSendArray[16] = buffer[16];	//震动
		mSendArray[17] = buffer[17];	//震动
		mSendArray[18] = buffer[18];	//震动
		mSendArray[19] = buffer[19];	//震动

		mSendArray[20] = buffer[20];	//进度灯(boss用)
		mSendArray[21] = buffer[21];	//闪光灯

		mSendArray[22] = buffer[22];	//开始灯
		mSendArray[35] = buffer[35];	//开始灯

		mSendArray[36] = buffer[36];	//气囊上
		mSendArray[37] = buffer[37];	//气囊下
		mSendArray[40] = buffer[40];	//气囊左
		mSendArray[41] = buffer[41];	//气囊右

		mSendArray[43] = buffer[43];	//1P激光器控制
		mSendArray[44] = buffer[44];	//2P激光器控制

		mSendArray[54] = buffer[54];	//1P风扇
		mSendArray[55] = buffer[55];	//2P风扇
		
		BYTE buffer1[HID_BUFFER_LEN];
		ZeroMemory(buffer1 , HID_BUFFER_LEN);
		CopyMemory( buffer1, mSendArray, sizeof( mSendArray ) );
		//Con::printf("[4]=%x,[5]=%x,[6]=%x,[16]=%x,[17]=%x,[18]=%x,[19]=%x,[20]=%x,[21]=%x,[36]=%x,[37]=%x,[40]=%x,[41]=%x,[42]=%x,[43]=%x,[44]=%d",buffer1[4],buffer1[5],buffer1[6],buffer1[16],buffer1[17], buffer1[18],buffer1[19],buffer1[20],buffer1[21],buffer1[36], buffer1[37],buffer1[40], buffer1[41],buffer1[42], buffer1[43],buffer1[44]);
		BYTE test;
		S32 len = HidDevice_GetOutputReportBufferLength(gTestHidPtr);
		test = HidDevice_SetOutputReport_Interrupt(gTestHidPtr, buffer1 , len);
	}
}

void pcvr::setRandomValue(byte *buffer)
{
	//几个特殊的值
	mSendArray[0] = 0x02;
	mSendArray[1] = 0x55;
	mSendArray[2] = 0x42;	//字头1
	mSendArray[3] = 0x54;	//字头2

	//先给其余值取随机数，再有些需要赋其他值时候再进行覆盖
	for ( int i = 4; i < HID_BUFFER_LEN+2; i++ )
	{
		mSendArray[i] = gRandGen.randI( 0, 255 );
	}

	//4为减币条件
	//5 6分别为1 2P减币值
	if (mPlayerCanSubCoin[0] || mPlayerCanSubCoin[1])
	{
		mSendArray[4] = 0xaa;
	}
	else
	{
		mSendArray[4] = 0x00;
	}

	if (mPlayerCanSubCoin[0])
	{
		mPlayerCanSubCoin[0] = false;
		mSendArray[5] = mPlayerSubCoinNum[0];
	}
	else
	{
		mSendArray[5] = 0;
	}

	if (mPlayerCanSubCoin[1])
	{
		mPlayerCanSubCoin[1] = false;
		mSendArray[6] = mPlayerSubCoinNum[1];
	}
	else
	{
		mSendArray[6] = 0;
	}

	//7 8 9 10 11    13 14 15为随机数
	//12为以上随机数的异或值
	mSendArray[12] = 0x00;
	for (int j = 7; j < 16; j++)
	{
		if (j != 12)
		{
			mSendArray[12] ^= mSendArray[j];
		}
	}
	
	//16 17 18 19为震动设置，之后会被原来值覆盖掉

	//20 为进度灯控制

	//21 为闪光灯控制，自行控制灯的灭

	//22 1P开始灯控制 暂时未用
	mSendArray[21] = buffer[21];	//闪光灯
	mSendArray[22] = buffer[22];
	mSendArray[35] = buffer[35];

	mSendArray[36] = buffer[36];	//气囊上
	mSendArray[37] = buffer[37];	//气囊下

	//23 24 25 26    28 29 30 31 32 33 34为随机数
	//27为以上随机数的异或值
	mSendArray[27] = 0x00;
	for (int j = 21; j < 40; j++)
	{
		if (j != 27)
		{
			mSendArray[27] ^= mSendArray[j];
		}
	}

	//35 2P开始灯控制 暂时未用

	//36 37 40 41气囊上、下、左、右，会被覆盖掉

	//38 39 42 为随机值

	//43 44 为1P和2P激光器控制，会被覆盖掉

	//45	校验有效位
	//46	密码2
	//47 48为随机值
	//49	密码1
	//50	密码3
	//51	校验随机值1
	//52	校验随机值2
	//53	为随机值
	//54 55	1P和2P风扇，会被覆盖掉

	//56 57 58 59 60 为随机值

	mSendArray[54] = buffer[54];	//1P风扇
	mSendArray[55] = buffer[55];	//2P风扇

	//61	53到60的异或结果
	mSendArray[61] = 0x00;
	for (int j = 53; j < 61; j++)
	{
		mSendArray[61] ^= mSendArray[j];
	}
}
bool pcvr::getMessage()
{
	if ( !HidDevice_IsOpened( gTestHidPtr ) )
	{
		Con::printf("HidDevice is not opened, try to open device again!");
		openDevice();
		return false;
	}

	WORD reportSize		= HidDevice_GetOutputReportBufferLength( gTestHidPtr );
	DWORD	numReports			= HidDevice_GetMaxReportRequest( gTestHidPtr );
	DWORD	reportBufferSize	= numReports * reportSize;
	BYTE*	reportBuffer		= new BYTE[reportBufferSize];
	DWORD	reportBufferRead	= 0;
	BYTE	status;


	status = HidDevice_GetInputReport_Interrupt(gTestHidPtr,  reportBuffer, reportBufferSize, 1, &reportBufferRead);

	if ( status == HID_DEVICE_SUCCESS || status == HID_DEVICE_TRANSFER_TIMEOUT )
	{
		BYTE buffer[HID_BUFFER_LEN];
		ZeroMemory(buffer , sizeof(buffer) );
		int count = reportBuffer[1];
		int index = 0;
		for (int i = 0 ; i < sizeof(buffer); ++i )
		{
			buffer[index] = reportBuffer[i + 2];
			++index;
		}
		if ( count >= HID_BUFFER_LEN )
		{
			CopyMemory( mGetArray, buffer, sizeof( mGetArray) );
			
			if ( gPcvr )
			{
				gPcvr->keyProcess();
			}
			/*Con::warnf("GetMessage:");
			printArray( mGetArray, sizeof( mGetArray ) );*/
			delete [] reportBuffer;
			return true;
		}
		else
			return false;
	}
	else
	{
		Con::executef( "OnConnectState", "3" );
		delete [] reportBuffer;
		//deleteGun();
		openDevice();
		return false;
	}
}

void pcvr::setRockerState( bool state )
{
	mNeedRocker = state;
}

void pcvr::openFlashLight( bool flash )
{
	if (flash)
	{
		mSendArray[21] =( 0xaa );
	}
	else
	{
		mSendArray[21] =( 0x55 );
	}
}

void pcvr::setFanRotateState( S32 playerIndex, bool state )
{
	if (state)
	{
		mSendArray[54+playerIndex] = 0xaa;
	}
	else
	{
		mSendArray[54+playerIndex] = 0x55;
	}
}

void pcvr::setSendScriptsState( bool isSend )
{
	mSendHidMsgToScripts = isSend;
}

////////////////////////////consolefunciton/////////////////////////////////////
ConsoleFunction( PCVRSetSendMsg, void, 3, 3, "set pcvr send array msg" )
{
	if ( gPcvr != NULL )
	{
		gPcvr->setSendMsg( dAtoi(argv[1]), dAtoi(argv[2]) );
	}
}

ConsoleFunction( PCVRSubPlayerCoin, void, 3, 3, "sub player coin num" )
{
	if ( gPcvr != NULL )
	{
		gPcvr->subPlayerCoin(dAtoi(argv[1]), dAtoi(argv[2] ) );
	}
}

//get player coin
ConsoleFunction( PCVRGetPlayerCoin, S32, 1, 1, "get player coin num " )
{
	if ( gPcvr != NULL )
	{
		S32 num = gPcvr->getCoinNum();
		return num;
	}
	else
		return 0;
}

ConsoleFunction( PCVRSetProcessLightLevel, void, 2, 2, "set boss damage level" )
{
	if( gPcvr != NULL )
	{
		gPcvr->setProcessLightLevel( dAtoi(argv[1] ) );
	}
}

ConsoleFunction( PCVRFlashPlayerStartLight, void, 3, 3, "" )
{
	if ( gPcvr != NULL )
	{
		gPcvr->flashPlayerStartLight( dAtoi(argv[1]), dAtob(argv[2]) );
	}
}

ConsoleFunction( PCVRSetBasePoint, void, 2, 2, "set basePoint" )
{
	if ( gPcvr != NULL )
	{
		gPcvr->setBasePoint(dAtoi(argv[1]) );
	}
}

ConsoleFunction( PCVROpenPlayerGun, void, 3, 3, "open the gun" )
{
	if ( gPcvr != NULL )
	{
		gPcvr->openPlayerGun(dAtoi(argv[1]),dAtob( argv[2] ) );
	}
}

ConsoleFunction( PCVRSetRockerState, void, 2, 2, "set whether need rocker" )
{
	if ( gPcvr != NULL )
	{
		gPcvr->setRockerState(dAtob(argv[1]));
	}
}

ConsoleFunction( PCVROpenFlashLight, void, 2, 2, "open the flash light" )
{
	if( gPcvr != NULL )
	{
		gPcvr->openFlashLight(dAtob(argv[1]));
	}
}

ConsoleFunction( PCVRSetFanRotateState, void, 3, 3, "open or close the fan" )
{
	if ( gPcvr != NULL )
	{
		gPcvr->setFanRotateState( dAtoi(argv[1]), dAtob(argv[2]) );
	}
}

ConsoleFunction( PCVRSetBallonetState, void, 3, 3, "set ballonet state")
{
	if ( gPcvr != NULL )
	{
		gPcvr->setBallonetState( dAtoi( argv[1] ), dAtob(argv[2] ) );
	}
}

ConsoleFunction(PCVRGetCrossPosition, const char*, 2, 2, "")
{
	int pcvrIndex = dAtoi( argv[1] );
	char *retBuffer = Con::getReturnBuffer(64);
	if ( pcvrIndex == 1 )
	{
		dSprintf(retBuffer, 64, "%d %d", gPcvrPointS0.x, gPcvrPointS0.y);
	}
	else if ( pcvrIndex == 2 )
	{
		dSprintf(retBuffer, 64, "%d %d", gPcvrPointS1.x, gPcvrPointS1.y);
	}
	else
		Con::errorf("invalid pcvr index!");

	return retBuffer;
}

ConsoleFunction( PCVRSetGunShakingState, void, 3, 3, "set the gun shake state")
{
	if ( gPcvr != NULL )
	{
		gPcvr->setGunShakeState( dAtoi( argv[1] ), dAtoi(argv[2] ) );
	}
}

ConsoleFunction( PCVRSetGunShakingLevel, void, 3, 3, "set the gun shake level")
{
	if ( gPcvr != NULL )
	{
		gPcvr->setShakeLevel( dAtoi( argv[1] ), dAtoi(argv[2] ) );
	}
}

ConsoleFunction( PCVRSetSendScriptsState, void, 2, 2, "")
{
	if ( gPcvr != NULL )
	{
		gPcvr->setSendScriptsState( dAtob( argv[1] ) );
	}
}

ConsoleFunction( PCVRSetPlayerCrossPos, void, 4, 4, "")
{
	if ( gPcvr != NULL )
	{
		if (dAtoi( argv[1] ) == 1)
		{
			gPcvrPointS0.x = dAtof( argv[2] );
			gPcvrPointS0.y = dAtof( argv[3] );
			gPcvrPointS0.z = 1.0f;
		}
		else if (dAtoi( argv[1] ) == 2)
		{
			gPcvrPointS1.x = dAtof( argv[2] );
			gPcvrPointS1.y = dAtof( argv[3] );
			gPcvrPointS1.z = 1.0f;
		}
	}
}
