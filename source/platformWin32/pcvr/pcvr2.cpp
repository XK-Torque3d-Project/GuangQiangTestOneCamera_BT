
//动感串口

#include "pcvr.h"
#include <iostream>
//#include "SerialPort.h"

//////////////////////////////////////
#include "T3D/shapeBase.h"
#include "PlayCapRectify.h"
#include "SampleGrabberCB.h"
#include "T3D/gameBase/gameConnection.h"
#include "math/mRandom.h"
#include "platform/platform.h"
#include <time.h>
#include "platformWin32/pcvr/SerialPort.h"

#pragma comment(lib, "SLABHIDDevice.lib")

pcvr* gPcvr = NULL;

bool			playerOneGunIsable = false;
bool			playerTwoGunIsable = false;

static  U32		CoinCurPcvr = 0;
static	U32		gOldCoinNum = 0;
U32				mOldCoinNum = 0;
U32				CoinCurGame = 0;
U32				SubCoinNum = 0;

static	U32		gP1NotFireSendTimes = 0;
static  U32		gP2NotFireSendTimes = 0;

static byte JiaoYanFailedMax = 0x02;
static byte JiaoYanSucceedCount = 0;
static byte JiaoYanFailedCount = 0;

const int tempLen = 4;
byte JiaoYanDt[tempLen];
byte JiaoYanMiMa[tempLen];
byte JiaoYanMiMaRand[tempLen];

U32 jiOuJiaoYanState = 0;	//0 - not jiaoyan; 1 - sucess; 2 - failed
byte JiOuJiaoYanFailCount = 0;
byte JiOuJiaoYanSucCount = 0;
byte JiOuJiaoYanMax = 2;

byte EndRead_1 = 0x41;
byte EndRead_2 = 0x42;
bool IsJiaoYanHidPcvr = false;
int jiaoyanSpaceTime = 30000;	//30s
int jiaoyanTotalTime = 5000;	//30s
int jiaoyanBeginTime = 0;
bool stopJiaoyan = false;
bool bJiaoyanFailed = false;
int jiaoyan1Count = 0;

BYTE	gunShakeLevelP1 = 0;
BYTE	gunShakeLevelP2 = 0;

//void CALLBACK TimerProc(HWND hWnd,UINT nMsg,UINT nTimerid,DWORD dwTime); 


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
	rootCanvas = NULL;
	oldHitCtrl0Id = -1;
	oldHitCtrl1Id = -1;

	m_disConnectTime = 0;
	m_tempCount = 0;
	isMouse1Up = true;
	isMouse2Up = true;
	b_setEnterKeyDown = false;
	b_setMoveKeyDown = false;
	b_playerOneOnFire = false;
	b_playerTwoOnFire = false;
	b_playerOneStartKeyDown = false;
	b_playerTwoStartKeyDown = false;
	b_jiaotabanOneDown = false;
	b_jiaotabanTwoDown = false;
	b_yaoganUp = false;
	b_yaoganDown = false;
	b_yaoganLeft = false;
	b_yaoganRight = false;
	b_playerShakeP1 = false;
	b_playerShakeP2 = false;
	IsCleanHidCoin = false;
	b_enable = false;
	m_equal.set( 5, 5 );

	b_BeifenAnjian1 = false;
	b_BeifenAnjian2 = false;
	b_BeifenAnjian3 = false;
	b_BeifenAnjian4 = false;
	b_BeifenAnjian5 = false;
	b_BeifenAnjian6 = false;

	isShanguangdeng = false;
	shanguangdengCount = 0;

	mSendHidMsgToScripts = false;
	mNeedRocker = false;
	BossLightValue = 0x00;

	mCameraEnable = false;

	ZeroMemory( m_sendArray, sizeof( m_sendArray ) );
	ZeroMemory( m_sendArrTemp5, sizeof( m_sendArrTemp5 ) );
	ZeroMemory( m_sendArrTemp6, sizeof( m_sendArrTemp6 ) );
	ZeroMemory( m_sendArrTemp7, sizeof( m_sendArrTemp7 ) );
	ZeroMemory( gGetMsg, sizeof( gGetMsg ) );
	m_saveCoord[0].set( 0, 0 );
	m_saveCoord[1].set( 0, 0 );
	resetBasePoint(0);
	ZeroMemory( mOnSetBasePointState, sizeof(mOnSetBasePointState) );
	ZeroMemory( mSetBasePointKeyDown, sizeof(mSetBasePointKeyDown) );

	ZeroMemory( JiaoYanDt, sizeof( JiaoYanDt ) );
	ZeroMemory( JiaoYanMiMa, sizeof( JiaoYanMiMa ) );
	ZeroMemory( JiaoYanMiMaRand, sizeof( JiaoYanMiMaRand ) );
	
	InitJiaoYanMiMa();
	
	return true;
}

bool pcvr::createGun()
{
		if (true)	//fffffffffffffffffffff 看着两个到底是啥
		{
			Con::executef( "OnConnectState", "2" );
			return false;
		}
		else
		{
			Con::executef( "OnConnectState", "1" );
			createCamera();
			return true;
		}
	return false;
}

bool pcvr::deleteGun()
{//ffffffffffffffffffffffffffffff 什么时候调用
	releaseCamera();
	return true;
}

bool pcvr::openDevice()
{//ffffffffffffffffffffffffffff
	if (true)
	{
		return false;
	}
	else
	{
		Con::executef( "OnConnectState", "1" );
		return true;
	}
	return false;
}

void pcvr::Enable(void)
{
	b_enable = true;
}

void pcvr::Disable(void)
{
	b_enable = false;
}

void pcvr::keyProcess()
{
	if ( !b_enable )
	{
		return;
	}

	//奇偶校验
	if (jiOuJiaoYanState == 0 && (gGetMsg[34]&0x01) == 0x01)
	{
		JiOuJiaoYanFailCount++;
		if (JiOuJiaoYanFailCount >= JiOuJiaoYanMax)
		{
			jiOuJiaoYanState = 2;
			Con::errorf("j    *****************     jo f");

			SetjiaoyanResult(false);
		}
	}
	else if (jiOuJiaoYanState == 0 && (gGetMsg[34]&0x01) == 0x00)
	{
		JiOuJiaoYanSucCount ++;

		if (JiOuJiaoYanSucCount >= JiOuJiaoYanMax)
		{
			jiOuJiaoYanState = 1;
			resetBasePoint(0);
			Con::errorf("j    *****************     jo s");
		}
	}

	if (IsJiaoYanHidPcvr)
	{//加密校验
		byte tmpVal = 0x00;
		for (int i = 26; i < 29; i++)
		{
			tmpVal ^= gGetMsg[i];
		}

		if (tmpVal == gGetMsg[25])
		{Con::errorf("j    1*****************      s");
			bool isJiaoYanDtSucceed = false;

			tmpVal = 0x00;
			for (int i = 30; i < 33; i++)
			{
				tmpVal ^= gGetMsg[i];
			}

			//校验2...
			if ( tmpVal == gGetMsg[29]
			&& (JiaoYanDt[1]&0xef) == gGetMsg[30]
			&& (JiaoYanDt[2]&0xfe) == gGetMsg[31]
			&& (JiaoYanDt[3]|0x28) == gGetMsg[32] )
			{Con::errorf("j    2 jm*****************      s");
				isJiaoYanDtSucceed = true;
				OnEndJiaoYanIO(SUCCEED);
			}
			else
			{Con::errorf("j    2 jm*****************      f");
				//fail
				OnEndJiaoYanIO(FAILED);
			}
		}
		else
		{Con::errorf("j    1*****************      f");
			jiaoyan1Count ++;

			if (jiaoyan1Count > 2)
			{
				//fail
				OnEndJiaoYanIO(FAILED);
			}
		}
	}

	setCoord();

	//CoinCurPcvr = (U32)gGetMsg[18];
	CoinCurPcvr = (U32)(gGetMsg[18] & 0x0f);
	if (CoinCurPcvr > 0)
	{
		if (!IsCleanHidCoin)
		{
			IsCleanHidCoin = true;
			mOldCoinNum += CoinCurPcvr;
			CoinCurGame = mOldCoinNum;

			//change coin information here
			Con::errorf("player coin num:  %d", CoinCurGame);
			sprintf(coinStr, "%d",CoinCurPcvr);
			Con::executef( "insertCoin", coinStr );
			
			if (bJiaoyanFailed)
			{
				Con::executef( "jiaoyanFailed",  "Failed" );
			}
		}
	}

	bool isQianMingAwake = false;
	GuiControl *p = NULL;
	if ( Sim::findObject( "signatureGui", p ) )
	{
		if ( p->isAwake() )
		{
			isQianMingAwake = true;
		}
	}
	bool isSelectLevelGuiAwake = false;
	if( Sim::findObject( "selectLevelGui", p ) )
	{
		if ( p->isAwake() )
		{
			isSelectLevelGuiAwake = true;
		}
	}
	if ( !isQianMingAwake )
	{
		//玩家1开枪
		if( ( gGetMsg[23] & 0x01 ) == 0x01
			&& !b_playerOneOnFire
			&& !isSelectLevelGuiAwake )
		{
			b_playerOneOnFire = true;
			gP1NotFireSendTimes = 0;
			Con::executef( "playerOnePressShoot", "1" );

			if ( mSendHidMsgToScripts )
			{
				char *retBuffer = Con::getReturnBuffer(10);
				dSprintf(retBuffer, 10, "%d", 0 );
				Con::executef("onChooseButtonDown", retBuffer );
			}
		}
		else if( ( gGetMsg[23] & 0x01 ) == 0x00 )
		{
			if ( gP1NotFireSendTimes < 10 )
			{
				gP1NotFireSendTimes++;
				Con::executef( "playerOnePressShoot",  "0" );
			}
			if ( mSendHidMsgToScripts && b_playerOneOnFire )
			{
				char *retBuffer = Con::getReturnBuffer(10);
				dSprintf(retBuffer, 10, "%d", 0 );
				Con::executef("onChooseButtonUp", retBuffer );
			}
			b_playerOneOnFire = false;
		}

		//玩家1开始--ok
		if( !b_playerOneStartKeyDown
			&& ( gGetMsg[23] & 0x08 ) == 0x08 )
		{
			b_playerOneStartKeyDown = true;
			Con::executef( "startPlayer1Game", "1" );

			if (bJiaoyanFailed)
			{
				Con::executef( "jiaoyanFailed",  "Failed" );
			}
		}
		else if ( ( gGetMsg[23] & 0x08 ) == 0x00 )
		{
			if(b_playerOneStartKeyDown)
			{
				Con::executef( "startPlayer1Game", "0" );
			}
			b_playerOneStartKeyDown = false;
		}

		//玩家2开枪
		if( ( gGetMsg[23] & 0x40) == 0x40
			&& !b_playerTwoOnFire
			&& !isSelectLevelGuiAwake )
		{
			b_playerTwoOnFire = true;
			gP2NotFireSendTimes = 0;
			Con::executef( "playerTwoPressShoot", "1" );
			if ( mSendHidMsgToScripts )
			{
				char *retBuffer = Con::getReturnBuffer(10);
				dSprintf(retBuffer, 10, "%d", 0 );
				Con::executef("onChooseButtonDown", retBuffer );
			}
		}
		else if( ( gGetMsg[23] & 0x40 ) == 0x00 )
		{
			if ( gP2NotFireSendTimes < 10 )
			{
				gP2NotFireSendTimes++;
				Con::executef( "playerTwoPressShoot", "0" );
			}
			if ( mSendHidMsgToScripts && b_playerTwoOnFire )
			{
				char *retBuffer = Con::getReturnBuffer(10);
				dSprintf(retBuffer, 10, "%d", 1 );
				Con::executef("onChooseButtonUp", retBuffer );
			}
			b_playerTwoOnFire = false;
		}

		//玩家2开始--ok
		if( !b_playerTwoStartKeyDown
			&& ( gGetMsg[24] & 0x02 ) == 0x02 )
		{
			b_playerTwoStartKeyDown = true;
			Con::executef( "startPlayer2Game", "1" );

			if (bJiaoyanFailed)
			{
				Con::executef( "jiaoyanFailed",  "Failed" );
			}
		}
		else if ( ( gGetMsg[24] & 0x02 ) == 0x00 )
		{
			if(b_playerTwoStartKeyDown)
			{
				Con::executef( "startPlayer2Game", "0" );
			}
			b_playerTwoStartKeyDown = false;
		}

		//脚踏板信息--ok
		if( !b_jiaotabanOneDown
			&& ( gGetMsg[23] & 0x04 ) == 0x04 )
		{
			b_jiaotabanOneDown = true;
			Con::executef( "playerOneOnButtonDown", "0" );
		}
		else if ( ( gGetMsg[23] & 0x04 ) == 0x00 )
		{
			if(b_jiaotabanOneDown)
			{
				Con::executef( "playerOneOnButtonDown", "1" );
			}
			b_jiaotabanOneDown = false;
		}

		if( !b_jiaotabanTwoDown
			&& ( gGetMsg[24] & 0x01 ) == 0x01 )
		{
			b_jiaotabanTwoDown = true;
			Con::executef( "playerTwoOnButtonDown", "0" );
		}
		else if ( ( gGetMsg[24] & 0x01 ) == 0x00 )
		{
			if(b_jiaotabanTwoDown)
			{
				Con::executef( "playerTwoOnButtonDown", "1" );
			}
			b_jiaotabanTwoDown = false;
		}

		//摇杆--ok
		if (mNeedRocker)
		{
			if( !b_yaoganUp && 
				!isSelectLevelGuiAwake 
				&& ( gGetMsg[23] & 0x10 ) == 0x10 )
			{
				b_yaoganUp = true;
				//Con::executef( "playerOnePressRocket", "1" );
			}
			else if( ( gGetMsg[23] & 0x10 ) == 0x00 )
			{
				if ( b_yaoganUp )
				{
					Con::executef( "PDDunpaiMoveU", "0" );
				}
				b_yaoganUp = false;
			}

			if( !b_yaoganDown && 
				!isSelectLevelGuiAwake 
				&& ( gGetMsg[23] & 0x20 ) == 0x20 )
			{
				b_yaoganDown = true;
				//Con::executef( "playerTwoPressRocket", "1" );
			}
			else if( b_yaoganDown
				&& ( gGetMsg[23] & 0x20 ) == 0x00 )
			{
				if ( b_yaoganDown )
				{
					Con::executef( "PDDunpaiMoveD", "0" );
				}
				b_yaoganDown = false;	
			}

			if( !b_yaoganLeft && 
				!isSelectLevelGuiAwake 
				&& ( gGetMsg[24] & 0x04 ) == 0x04 )
			{
				b_yaoganLeft = true;
				//Con::executef( "playerOnePressRocket", "1" );
			}
			else if( ( gGetMsg[24] & 0x04 ) == 0x00 )
			{
				if ( b_yaoganLeft )
				{
					Con::executef( "PDDunpaiMoveL", "0" );
				}
				b_yaoganLeft = false;
			}

			if( !b_yaoganRight && 
				!isSelectLevelGuiAwake 
				&& ( gGetMsg[24] & 0x08 ) == 0x08 )
			{
				b_yaoganRight = true;
				//Con::executef( "playerTwoPressRocket", "1" );
			}
			else if( b_yaoganRight
				&& ( gGetMsg[24] & 0x08) == 0x00 )
			{
				if ( b_yaoganRight )
				{
					Con::executef( "PDDunpaiMoveR", "0" );
				}
				b_yaoganRight = false;	
			}

			if (b_yaoganUp && !b_yaoganDown && !b_yaoganLeft && !b_yaoganRight)
			{
				Con::executef( "PDDunpaiMoveU", "1" );
			}
			else if (!b_yaoganUp && b_yaoganDown && !b_yaoganLeft && !b_yaoganRight)
			{
				Con::executef( "PDDunpaiMoveD", "1" );
			}
			else if (!b_yaoganUp && !b_yaoganDown && b_yaoganLeft && !b_yaoganRight)
			{
				Con::executef( "PDDunpaiMoveL", "1" );
			}
			else if (!b_yaoganUp && !b_yaoganDown && !b_yaoganLeft && b_yaoganRight)
			{
				Con::executef( "PDDunpaiMoveR", "1" );
			}
			else if (b_yaoganUp && !b_yaoganDown && b_yaoganLeft && !b_yaoganRight)
			{
				Con::executef( "PDDunpaiMoveLU", "1" );
			}
			else if (b_yaoganUp && !b_yaoganDown && !b_yaoganLeft && b_yaoganRight)
			{
				Con::executef( "PDDunpaiMoveRU", "1" );
			}
			else if (!b_yaoganUp && b_yaoganDown && b_yaoganLeft && !b_yaoganRight)
			{
				Con::executef( "PDDunpaiMoveLD", "1" );
			}
			else if (!b_yaoganUp && b_yaoganDown && !b_yaoganLeft && b_yaoganRight)
			{
				Con::executef( "PDDunpaiMoveRD", "1" );
			}
		}
		
		//设置界面--ok
		if( !b_setEnterKeyDown
			&& ( gGetMsg[21] & 0x40 ) == 0x40 )
		{
			b_setEnterKeyDown = true;
			Con::executef( "onIntoGameSetGui",  "0" );
		}
		else if ( ( gGetMsg[21] & 0x40 ) == 0x00 )
		{
			b_setEnterKeyDown = false;
		}

		if ( !b_setMoveKeyDown
			&& (gGetMsg[21] & 0x80 ) == 0x80 )
		{
			b_setMoveKeyDown = true;
			Con::executef( "onIntoGameSetGui",  "1" );
		}
		else if( ( gGetMsg[21] & 0x80 ) == 0x00 )
		{
			b_setMoveKeyDown = false;
		}

		if ( !isSelectLevelGuiAwake )
		{
			ZeroMemory( gGetMsg, sizeof( gGetMsg ) );
		}
	}
	
	camSetBasePoint();
}

int pcvr::getCoinNum()
{
	return (int)CoinCurGame;
}

void pcvr::subCoinCount( int playerIndex, int subNum )
{
	if ( subNum < 0 )
	{
		mOldCoinNum += (U32)(0 - subNum);
		CoinCurGame = mOldCoinNum;
		return;
	}

	U32 subNumTemp = (U32)subNum;
	
	if (gOldCoinNum >= subNumTemp) {
		gOldCoinNum = gOldCoinNum - subNumTemp;
	}
	else {
		SubCoinNum = subNumTemp - gOldCoinNum;
		if (mOldCoinNum < SubCoinNum) {
			return;
		}

		mOldCoinNum -= SubCoinNum;
		CoinCurGame = mOldCoinNum;
		gOldCoinNum = 0;
	}
}

void pcvr::flashStartLight( int index, int startIndex )
{
	if ( index == 1 )
	{
		if( startIndex == 1 )
		{
			m_sendArrTemp5[0] = 0x01;
		}
		else if( startIndex == 0 )
		{
			m_sendArrTemp5[0] = 0x00;
		}
	}
	else if ( index == 2 )
	{
		if( startIndex == 1 )
		{
			m_sendArrTemp5[1] = 0x01;
		}
		else if( startIndex == 0 )
		{
			m_sendArrTemp5[1] = 0x00;
		}
	}
	else if( index == 0 )
	{
		if( startIndex == 1 )
		{
			m_sendArrTemp5[0] = 0x01;
			m_sendArrTemp5[1] = 0x01;
		}
		else if( startIndex == 0 )
		{
			m_sendArrTemp5[0] = 0x00;
			m_sendArrTemp5[1] = 0x00;
		}
	}
}

void pcvr::setPlayerHIDSpeed( int playerIndex, int speedIndex )
{Con::errorf("setPlayerHIDSpeed  %d  %d", playerIndex, speedIndex );
	if (playerIndex == 0)
	{
		if (speedIndex <= 0)
		{
			b_playerShakeP1 = false;
			b_playerShakeP2 = false;
		}
		else
		{
			b_playerShakeP1 = true;
			b_playerShakeP2 = true;
		}
	}
	else if (playerIndex == 1)
	{
		if (speedIndex <= 0)
		{
			b_playerShakeP1 = false;
		}
		else
		{
			b_playerShakeP1 = true;
		}
	}
	else if (playerIndex == 2)
	{
		if (speedIndex <= 0)
		{
			b_playerShakeP2 = false;
		}
		else
		{
			b_playerShakeP2 = true;
		}
	}
}

void pcvr::setShakeLevel( U32 playerIndex, U32 levelIndex )
{Con::errorf("setHidShakeLevelsetHidShakeLevel  %d  %d", playerIndex, levelIndex );
	if (levelIndex <= 1)
	{
		levelIndex = 0;
	}
	else
	{
		levelIndex = (U32)(255 / 14 * (levelIndex - 1));
	}

	if ( playerIndex == 1 )
	{
		gunShakeLevelP1 = (BYTE)levelIndex;
	}
	else if ( playerIndex == 2 )
	{
		gunShakeLevelP2 = (BYTE)levelIndex;
	}
	else if ( playerIndex == 0 )
	{
		gunShakeLevelP1 = (BYTE)levelIndex;
		gunShakeLevelP2 = (BYTE)levelIndex;
	}
}

//
void pcvr::sendMoveMessage(int pIndex, int messageIndex )
{
	if ( messageIndex < 0 || messageIndex > 19 )
	{
		Con::errorf("error message index %d", messageIndex );
	}
	else if ( messageIndex < 10)
	{
		if ( messageIndex == 1 )
		{
			m_sendArrTemp6[0] = 0x01;
		}
		else if ( messageIndex == 2 )
		{
			m_sendArrTemp6[1] = 0x01;
		}
		else if ( messageIndex == 3 )
		{
			m_sendArrTemp6[2] = 0x01;
		}
		else if ( messageIndex == 4 )
		{
			m_sendArrTemp6[3] = 0x01;
		}
		else if ( messageIndex == 6)
		{
			m_sendArrTemp6[0] = 0x00;
		}
		else if ( messageIndex == 7)
		{
			m_sendArrTemp6[1] = 0x00;
		}
		else if ( messageIndex == 8 )
		{
			m_sendArrTemp6[2] = 0x00;
		}
		else if ( messageIndex == 9 )
		{
			m_sendArrTemp6[3] = 0x00;
		}
	}
	else
	{
		if ( messageIndex == 11 )
		{
			m_sendArrTemp6[4] = 0x01;
		}
		else if ( messageIndex == 12 )
		{
			m_sendArrTemp6[5] = 0x01;
		}
		else if ( messageIndex == 13 )
		{
			m_sendArrTemp6[6] = 0x01;
		}
		else if ( messageIndex == 14 )
		{
			m_sendArrTemp6[7] = 0x01;
		}
		else if ( messageIndex == 16)
		{
			m_sendArrTemp6[4] = 0x00;
		}
		else if ( messageIndex == 17)
		{
			m_sendArrTemp6[5] = 0x00;
		}
		else if ( messageIndex == 18 )
		{
			m_sendArrTemp6[6] = 0x00;
		}
		else if ( messageIndex == 19 )
		{
			m_sendArrTemp6[7] = 0x00;
		}
	}
}

void pcvr::Process()
{
	if ( !b_enable )
	{	
		return;
	}

	bool isQianMingAwake = false;
	GuiControl *p = NULL;
	if ( Sim::findObject( "signatureGui", p ) )
	{
		if ( p->isAwake() )
		{
			isQianMingAwake = true;
		}
	}
	bool isSelectLevelGuiAwake = false;
	if( Sim::findObject( "selectLevelGui", p ) )
	{
		if ( p->isAwake() )
		{
			isSelectLevelGuiAwake = true;
		}
	}
	if ( isQianMingAwake || isSelectLevelGuiAwake )
	{
		GuiControl *p = NULL;
		if ( !Sim::findObject( "Canvas", p ) )
		{
			Con::errorf("there is something wrong with rootCanvas");
		}
		else
		{
			GuiControl* rootCanvas = dynamic_cast<GuiCanvas*>(p);
			if ( rootCanvas == NULL )
			{
				Con::errorf("root canvas is NULL");
				return;
			}
			
			/*if ( !playerOneGunIsable )
			{
				pt_currentPot0.set( 0, 0 );
			}
			if ( !playerTwoGunIsable )
			{
				pt_currentPot1.set( 0, 0 );
			}*/
			GuiControl *hitCtrl0 = NULL;
			GuiControl *hitCtrl1 = NULL;
			GuiControl *oldHitCtrl0 = NULL;
			GuiControl *oldHitCtrl1 = NULL;
			if ( oldHitCtrl0Id != -1 )
			{
				Sim::findObject( oldHitCtrl0Id, oldHitCtrl0 );
			}
			if ( oldHitCtrl1Id != -1 )
			{
				Sim::findObject( oldHitCtrl1Id, oldHitCtrl1 );
			}
			

			hitCtrl0 = rootCanvas->findHitControl( pt_currentPot0 );
			hitCtrl1 = rootCanvas->findHitControl( pt_currentPot1 );

			GuiEvent event0;
			GuiEvent event1;

			event0.mousePoint = pt_currentPot0;
			event1.mousePoint = pt_currentPot1;
			
			if ( hitCtrl0 != NULL && hitCtrl0 != oldHitCtrl0 )
			{
				hitCtrl0->onMouseEnter( event0 );
				Con::printf("hitCtrl0->onMouseEnter( event0 )");
			}
			if( hitCtrl1 != NULL && hitCtrl1 != oldHitCtrl1 )
			{
				hitCtrl1->onMouseEnter( event1 );
				Con::printf("hitCtrl1->onMouseEnter( event1 );");
			}
			if ( hitCtrl0 != NULL )
			{
				if ( hitCtrl0 != oldHitCtrl0 )
				{
					GuiEvent event;
					if ( hitCtrl0 != oldHitCtrl0 && oldHitCtrl0 != NULL )
					{
						Con::printf("oldHitCtrl0->onMouseLeave(event)1;");
						Con::printf("oldHitCtrl0's name is %s", oldHitCtrl0->getName() );
						oldHitCtrl0->onMouseUp(event);
						oldHitCtrl0->onMouseLeave(event);
						Con::printf("oldHitCtrl0->onMouseLeave(event)2;");
					}
				}

				if ( ( gGetMsg[19] & 0x40 ) == 0x40 )
				{
					if( isMouse1Up )
					{
						GuiEvent event;
						hitCtrl0->onMouseDown( event );
						isMouse1Up = false;
						Con::printf("hitCtrl0->onMouseDown( event );");
					}
				}
				else if( !isMouse1Up 
						&& ( gGetMsg[19] & 0x40 ) == 0x00 )
				{
					GuiEvent event;
					hitCtrl0->onMouseUp(event);
					isMouse1Up = true;
					Con::printf("hitCtrl0->onMouseUp(event);");
				}
				oldHitCtrl0Id = hitCtrl0->getId();
			}
			if ( hitCtrl1 != NULL )
			{
				if ( hitCtrl1 != oldHitCtrl1 )
				{
					GuiEvent event;
					if ( hitCtrl1 != oldHitCtrl1 && oldHitCtrl1 != NULL )
					{
						oldHitCtrl1->onMouseUp(event);
						oldHitCtrl1->onMouseLeave(event);
					}
				}

				if ( ( gGetMsg[20] & 0x01 ) == 0x01 )
				{
					if( isMouse2Up )
					{
						GuiEvent event;
						hitCtrl1->onMouseDown( event );
						isMouse2Up = false;
					}
				}
				else if(!isMouse2Up 
						&& ( gGetMsg[20] & 0x01 ) == 0x00 )
				{
					GuiEvent event;
					hitCtrl1->onMouseUp(event);
					isMouse2Up = true;
				}
				oldHitCtrl1Id = hitCtrl1->getId();
			}
		}
	}
	else
	{
		oldHitCtrl0Id = -1;
		oldHitCtrl1Id = -1;
	}
}

void pcvr::resetBasePoint( int index )
{
	if ( index == 1 || index == 0 )
	{
		pt_one_leftTop.x = GetPrivateProfileIntA("Position", "GunOnePosLeftTopX", 0, ".\\Pos.hnb");
		pt_one_leftTop.y = GetPrivateProfileIntA("Position", "GunOnePosLeftTopY", 0, ".\\Pos.hnb");
		pt_one_rightTop.x = GetPrivateProfileIntA("Position", "GunOnePosRightTopX", 0, ".\\Pos.hnb");
		pt_one_rightTop.y = GetPrivateProfileIntA("Position", "GunOnePosRightTopY", 0, ".\\Pos.hnb");

		pt_one_rightBottom.x = GetPrivateProfileIntA("Position", "GunOnePosRightBottomX", 0, ".\\Pos.hnb");
		pt_one_rightBottom.y = GetPrivateProfileIntA("Position", "GunOnePosRightBottomY", 0, ".\\Pos.hnb");
		pt_one_leftBottom.x = GetPrivateProfileIntA("Position", "GunOnePosLeftBottomX", 0, ".\\Pos.hnb");
		pt_one_leftBottom.y = GetPrivateProfileIntA("Position", "GunOnePosLeftBottomY", 0, ".\\Pos.hnb");
	}
	if ( index == 2 || index == 0 )
	{
		pt_two_leftTop.x = GetPrivateProfileIntA("Position", "GunTwoPosLeftTopX", 0, ".\\Pos.hnb");
		pt_two_leftTop.y = GetPrivateProfileIntA("Position", "GunTwoPosLeftTopY", 0, ".\\Pos.hnb");
		pt_two_rightTop.x = GetPrivateProfileIntA("Position", "GunTwoPosRightTopX", 0, ".\\Pos.hnb");
		pt_two_rightTop.y = GetPrivateProfileIntA("Position", "GunTwoPosRightTopY", 0, ".\\Pos.hnb");
		pt_two_rightBottom.x = GetPrivateProfileIntA("Position", "GunTwoPosRightBottomX", 0, ".\\Pos.hnb");
		pt_two_rightBottom.y = GetPrivateProfileIntA("Position", "GunTwoPosRightBottomY", 0, ".\\Pos.hnb");
		pt_two_leftBottom.x = GetPrivateProfileIntA("Position", "GunTwoPosLeftBottomX", 0, ".\\Pos.hnb");
		pt_two_leftBottom.y = GetPrivateProfileIntA("Position", "GunTwoPosLeftBottomY", 0, ".\\Pos.hnb");
	}
}

void pcvr::setCoord()
{
	Point2I newPos;

	newPos.x = ( (gGetMsg[2] & 0x0f) << 8 ) + gGetMsg[3];
	newPos.y = ( (gGetMsg[4] & 0x0f) << 8 ) + gGetMsg[5];

	m_saveCoord[0] = newPos;

	if ( ( pt_one_rightBottom.x - pt_one_leftTop.x ) != 0  && ( pt_one_leftTop.y - pt_one_rightBottom.y ) != 0 )
	{
		if ( pt_one_leftTop.x < pt_one_rightBottom.x )
		{
			if( newPos.x < pt_one_leftTop.x )
			{
				newPos.x = pt_one_leftTop.x;
			}
			if ( newPos.x > pt_one_rightBottom.x )
			{
				newPos.x = pt_one_rightBottom.x;
			}
			float a = float( newPos.x - pt_one_leftTop.x) / float(pt_one_rightBottom.x - pt_one_leftTop.x);
			newPos.x = 1360 * a;
			if (pt_one_leftTop.y < pt_one_rightBottom.y)
			{
				if ( newPos.y < pt_one_leftTop.y )
				{
					newPos.y = pt_one_leftTop.y;
				}
				if( newPos.y > pt_one_rightBottom.y )
				{
					newPos.y = pt_one_rightBottom.y;
				}
				float b = float(newPos.y - pt_one_leftTop.y) / float(pt_one_rightBottom.y - pt_one_leftTop.y);
				newPos.y = 768 * b;
			}
			else
			{
				if ( newPos.y > pt_one_leftTop.y )
				{
					newPos.y = pt_one_leftTop.y;
				}
				if( newPos.y < pt_one_rightBottom.y )
				{
					newPos.y = pt_one_rightBottom.y;
				}
				float b = float(pt_one_leftTop.y - newPos.y) / float(pt_one_leftTop.y - pt_one_rightBottom.y);
				newPos.y = 768 * b;
			}
		}
		else
		{
			if( newPos.x > pt_one_leftTop.x )
			{
				newPos.x = pt_one_leftTop.x;
			}
			if ( newPos.x < pt_one_rightBottom.x )
			{
				newPos.x = pt_one_rightBottom.x;
			}
			float a = float(pt_one_leftTop.x - newPos.x) / float(pt_one_leftTop.x - pt_one_rightBottom.x);
			newPos.x = 1360 * a;
			if (pt_one_leftTop.y < pt_one_rightBottom.y)
			{
				if ( newPos.y < pt_one_leftTop.y )
				{
					newPos.y = pt_one_leftTop.y;
				}
				if( newPos.y > pt_one_rightBottom.y )
				{
					newPos.y = pt_one_rightBottom.y;
				}
				float b = float(newPos.y - pt_one_leftTop.y) / float(pt_one_rightBottom.y - pt_one_leftTop.y);
				newPos.y = 768 * b;
			}
			else
			{
				if ( newPos.y > pt_one_leftTop.y )
				{
					newPos.y = pt_one_leftTop.y;
				}
				if( newPos.y < pt_one_rightBottom.y )
				{
					newPos.y = pt_one_rightBottom.y;
				}
				float b = float(pt_one_leftTop.y - newPos.y) / float(pt_one_leftTop.y - pt_one_rightBottom.y);
				newPos.y = 768 * b;
			}
		}
	}

	if( newPos.x > 1360 )
	{
		newPos.x = 1360;
	}
	if (newPos.y > 768 )
	{
		newPos.y = 768;
	}
	if ( newPos.x < 0 )
	{
		newPos.x = 0;
	}
	if ( newPos.y < 0 )
	{
		newPos.y = 0;
	}

	if ( fabs( gPcvrPointS0.x - newPos.x ) > m_equal.x )
	{
		gPcvrPointS0.x = newPos.x;
	}
	if ( fabs( gPcvrPointS0.y - newPos.y ) > m_equal.y )
	{
		gPcvrPointS0.y = newPos.y;
	}
	gPcvrPointS0.z = 1.0f;

	newPos.x = ( (gGetMsg[6] & 0x0f) << 8 ) + gGetMsg[7];
	newPos.y = ( (gGetMsg[8] & 0x0f) << 8 ) + gGetMsg[9];

    m_saveCoord[1] = newPos;
	if ( ( pt_two_rightBottom.x - pt_two_leftTop.x ) != 0  && ( pt_two_leftTop.y - pt_two_rightBottom.y ) != 0 )
	{
		if ( pt_two_leftTop.x < pt_two_rightBottom.x )
		{
			if( newPos.x < pt_two_leftTop.x )
			{
				newPos.x = pt_two_leftTop.x;
			}
			if ( newPos.x > pt_two_rightBottom.x )
			{
				newPos.x = pt_two_rightBottom.x;
			}

			float a = float( newPos.x - pt_two_leftTop.x) / float(pt_two_rightBottom.x - pt_two_leftTop.x);
			newPos.x = 1360 * a;
			if (pt_two_leftTop.y < pt_two_leftBottom.y)
			{
				if ( newPos.y < pt_two_leftTop.y )
				{
					newPos.y = pt_two_leftTop.y;
				}
				if( newPos.y > pt_two_rightBottom.y )
				{
					newPos.y = pt_two_rightBottom.y;
				}
				float b = float(newPos.y - pt_two_leftTop.y) / float(pt_two_rightBottom.y - pt_two_leftTop.y);
				newPos.y = 768 * b;
			}
			else
			{
				if ( newPos.y > pt_two_leftTop.y )
				{
					newPos.y = pt_two_leftTop.y;
				}
				if( newPos.y < pt_two_rightBottom.y )
				{
					newPos.y = pt_two_rightBottom.y;
				}
				float b = float(pt_two_leftTop.y - newPos.y) / float(pt_two_leftTop.y - pt_two_rightBottom.y);
				newPos.y = 768 * b;
			}
		}
		else
		{
			if( newPos.x > pt_two_leftTop.x )
			{
				newPos.x = pt_two_leftTop.x;
			}
			if ( newPos.x < pt_two_rightBottom.x )
			{
				newPos.x = pt_two_rightBottom.x;
			}
			float a = float(pt_two_leftTop.x - newPos.x) / float(pt_two_leftTop.x - pt_two_rightBottom.x);
			newPos.x = 1360 * a;
			if (pt_two_leftTop.y < pt_two_rightBottom.y)
			{
				if ( newPos.y < pt_two_leftTop.y )
				{
					newPos.y = pt_two_leftTop.y;
				}
				if( newPos.y > pt_two_rightBottom.y )
				{
					newPos.y = pt_two_rightBottom.y;
				}
				float b = float(newPos.y - pt_two_leftTop.y) / float(pt_two_rightBottom.y - pt_two_leftTop.y);
				newPos.y = 768 * b;
			}
			else
			{
				if ( newPos.y > pt_two_leftTop.y )
				{
					newPos.y = pt_two_leftTop.y;
				}
				if( newPos.y < pt_two_rightBottom.y )
				{
					newPos.y = pt_two_rightBottom.y;
				}
				float b = float(pt_two_leftTop.y - newPos.y) / float(pt_two_leftTop.y - pt_two_rightBottom.y);
				newPos.y = 768 * b;
			}
		}
	}
	if( newPos.x > 1360 )
	{
		newPos.x = 1360;
	}
	if (newPos.y > 768 )
	{
		newPos.y = 768;
	}
	if ( newPos.x < 0 )
	{
		newPos.x = 0;
	}
	if ( newPos.y < 0 )
	{
		newPos.y = 0;
	}
	if ( fabs( gPcvrPointS1.x - newPos.x ) > m_equal.x )
	{
		gPcvrPointS1.x = newPos.x;
	}
	if ( fabs( gPcvrPointS1.y - newPos.y ) > m_equal.y )
	{
		gPcvrPointS1.y = newPos.y;
	}
	gPcvrPointS1.z = 1.0f;
}

void pcvr::sendMessage(bool flag)
{
	if (!stopJiaoyan && !IsJiaoYanHidPcvr && b_enable && flag && jiOuJiaoYanState == 1)
	{
		if (jiaoyanBeginTime <= 0)
		{
			jiaoyanBeginTime = Platform::getRealMilliseconds();
		}
		else if (Platform::getRealMilliseconds() - jiaoyanBeginTime >= jiaoyanSpaceTime)
		{
			StartJiaoYanIO();
			jiaoyanBeginTime = 0;
		}
	}

	ZeroMemory(gSendMsg, sizeof(gSendMsg));

	for(int i = 4; i < BufSendLen - 3; i++)
	{
		gSendMsg[i] = (byte)gRandGen.randI(0, 255);
	}

	//字头 字尾
	m_sendArray[0] = 0x02;
	m_sendArray[1] = 0x55;
	m_sendArray[BufSendLen - 2] = 0x0d;
	m_sendArray[BufSendLen - 1] = 0x0a;

	//2 - 减币命令
	//3 - 减币币值1
	if (IsCleanHidCoin)
	{
		m_sendArray[2] = 0xaa;
		m_sendArray[3] = 0x01;

		if (CoinCurPcvr == 0)
		{
			IsCleanHidCoin = false;
		}
	}
	else
	{
		m_sendArray[2] = 0x00;
		m_sendArray[3] = 0x00;
	}

	//减币-没用
	m_sendArray[4] = 0x00;

	if (isShanguangdeng)
	{
		shanguangdengCount ++;

		if (shanguangdengCount == 20)
		{
			m_sendArrTemp7[2] = 0x00;
			m_sendArrTemp7[3] = 0x00;
		}
		else if (shanguangdengCount == 40)
		{
			m_sendArrTemp7[2] = 0x01;
			m_sendArrTemp7[3] = 0x01;
			shanguangdengCount = 0;
		}
	}
	else
	{
		m_sendArrTemp7[2] = 0x00;
		m_sendArrTemp7[3] = 0x00;
	}

	//5 - 灯(0 1 开始灯)
	m_sendArray[5] = (BYTE)(m_sendArrTemp5[0] + m_sendArrTemp5[1] * 2 + m_sendArrTemp5[2] * 4 + m_sendArrTemp5[3] * 8
		+ m_sendArrTemp5[4] * 16 + m_sendArrTemp5[5] * 32 +m_sendArrTemp5[6] * 64 + m_sendArrTemp5[7] * 128);
	
	//6 - 气囊（0-7 ： 气囊1-气囊8） 位数组合 m_sendArrTemp6
	m_sendArray[6] = (BYTE)(m_sendArrTemp6[0] + m_sendArrTemp6[1] * 2 + m_sendArrTemp6[2] * 4 + m_sendArrTemp6[3] * 8
		+ m_sendArrTemp6[4] * 16 + m_sendArrTemp6[5] * 32 +m_sendArrTemp6[6] * 64 + m_sendArrTemp6[7] * 128);

	//7 - 灯(0 1 闪光灯)
	m_sendArray[7] = (BYTE)(m_sendArrTemp7[0] + m_sendArrTemp7[1] * 2 + m_sendArrTemp7[2] * 4 + m_sendArrTemp7[3] * 8
		+ m_sendArrTemp7[4] * 16 + m_sendArrTemp7[5] * 32 +m_sendArrTemp7[6] * 64 + m_sendArrTemp7[7] * 128);

	//震动等级 8 9
	if(!b_playerShakeP1)
	{
		m_sendArray[8] = 0x00;
	}
	else
	{
		m_sendArray[8] = gunShakeLevelP1;
	}

	if(!b_playerShakeP2)
	{
		m_sendArray[9] = 0x00;
	}
	else
	{
		m_sendArray[9] = gunShakeLevelP2;
	}

	//10随机值
	m_sendArray[10] = gSendMsg[10];

	//11 - 进度灯
	m_sendArray[11] = BossLightValue;

	//12-19随机值
	for(int j=12; j < 20; j++)
	{
		m_sendArray[j] = gSendMsg[j];
	}

	//20校验(2~~~11的异或)
	m_sendArray[20] = 0x00;

	for(int j=2; j<12; j++)
	{
		m_sendArray[20] ^= m_sendArray[j];
	}

	if (IsJiaoYanHidPcvr)
	{
		for (int i = 0; i < 4; i++)
		{
			m_sendArray[i + 21] = JiaoYanMiMa[i];
		}

		for (int i = 0; i < 4; i++) {
			m_sendArray[i + 25] = JiaoYanDt[i];
		}
	}
	else
	{
		RandomJiaoYanMiMaVal();

		for (int i = 0; i < 4; i++)
		{
			m_sendArray[i + 21] = JiaoYanMiMaRand[i];
		}

		for (int i = 26; i < 29; i++)
		{
			m_sendArray[i] = (byte)gRandGen.randI(0, 64);
		}

		m_sendArray[25] = 0x00;

		for (int i = 26; i < 29; i++)
		{
			m_sendArray[25] ^= m_sendArray[i];
		}
	}

	//6校验(2~~~11的异或)
	m_sendArray[6] = 0x00;

	for(int j=2; j<12; j++)
	{
		m_sendArray[6] ^= m_sendArray[j];
	}

	//18 20-随机值
	m_sendArray[18] = gSendMsg[18];
	m_sendArray[20] = gSendMsg[20];
	
	//29 - ^
	m_sendArray[29] = 0x00;

	for (int i = 0; i < BufSendLen; i++)
	{
		if (i == 29) 
		{
			continue;
		}

		m_sendArray[29] ^= m_sendArray[i];
	}
}

void pcvr::SetjiaoyanResult(bool flag)
{
	//找个地方通知脚本校验失败处理
	if (!flag)
	{
		bJiaoyanFailed = true;
		Con::executef( "jiaoyanFailed",  "Failed" );
	}
}

//校验
void pcvr:: InitJiaoYanMiMa()
{
	JiaoYanMiMa[1] = 0x8e;	//0x8e
	JiaoYanMiMa[2] = 0xc3;	//0xc3
	JiaoYanMiMa[3] = 0xd7;	//0xd7
	JiaoYanMiMa[0] = 0x00;
	for (int i = 1; i < 4; i++) {
		JiaoYanMiMa[0] ^= JiaoYanMiMa[i];
	}
}

void pcvr:: RandomJiaoYanDt()
{	
	for (int i = 1; i < 4; i++) {
		JiaoYanDt[i] = (byte)gRandGen.randI(0, 64);
	}
	JiaoYanDt[0] = 0x00;
	for (int i = 1; i < 4; i++) {
		JiaoYanDt[0] ^= JiaoYanDt[i];
	}
}

void pcvr:: RandomJiaoYanMiMaVal()
{
	for (int i = 0; i < 4; i++) {
		JiaoYanMiMaRand[i] = (byte)gRandGen.randI( 0, (int)(JiaoYanMiMa[i] - 1) );
	}

	byte TmpVal = 0x00;
	for (int i = 1; i < 4; i++) {
		TmpVal ^= JiaoYanMiMaRand[i];
	}

	if (TmpVal == JiaoYanMiMaRand[0]) {
		JiaoYanMiMaRand[0] = JiaoYanMiMaRand[0] == 0x00 ?
			(byte)gRandGen.randI( 1, 255 ) : (byte)(JiaoYanMiMaRand[0] + gRandGen.randI( 1, 255 ));
	}
}

void pcvr:: StartJiaoYanIO()
{
	if (IsJiaoYanHidPcvr || stopJiaoyan)
	{
		jiaoyanBeginTime = 0;
		return;
	}

	if (JiaoYanSucceedCount >= 1)
	{
		stopJiaoyan = true;
		return;
	}

	if (JiaoYanFailedCount >= JiaoYanFailedMax)
	{
		stopJiaoyan = true;
		return;
	}
	RandomJiaoYanDt();
	IsJiaoYanHidPcvr = true;
}

void pcvr:: OnEndJiaoYanIO(JIAOYANENUM val)
{
	switch (val)
	{
	case FAILED:
		JiaoYanFailedCount++;
		
		if (JiaoYanFailedCount >= JiaoYanFailedMax)
		{
			stopJiaoyan = true;

			SetjiaoyanResult(false);
		}
		break;

	case SUCCEED:
		JiaoYanSucceedCount++;
		JiaoYanFailedCount = 0;
		stopJiaoyan = true;

		break;
	}
	IsJiaoYanHidPcvr = false;
}

void pcvr::setRockerState( bool state )
{
	mNeedRocker = state;
}

void pcvr::setSendScriptsState( bool isSend )
{
	mSendHidMsgToScripts = isSend;
}

//进度灯
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
			BossLightValue = ( 0x10 + level);
		}
		else
		{
			BossLightValue = (0x10 + 0x0a);
		}
	}
}

//闪光灯
void pcvr::openFlashLight( bool flash )
{
	if (flash)
	{
		isShanguangdeng = true;
		shanguangdengCount = 0;

		m_sendArrTemp7[2] = 0x01;
		m_sendArrTemp7[3] = 0x01;
	}
	else
	{
		isShanguangdeng = false;

		m_sendArrTemp7[2] = 0x00;
		m_sendArrTemp7[3] = 0x00;
	}
}

//风扇
void pcvr::setFanRotateState( S32 playerIndex, bool state )
{
	if (state)
	{
		m_sendArrTemp7[playerIndex + 6] = 0x01;
	}
	else
	{
		m_sendArrTemp7[playerIndex + 6] = 0x00;
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
		if (val)
		{
			if (ballonetIndex == 0)
			{//4
				sendMoveMessage(1, 4);
			}
			else if (ballonetIndex == 1)
			{//2
				sendMoveMessage(1, 2);
			}
			else if (ballonetIndex == 2)
			{//1
				sendMoveMessage(1, 1);
			}
			else if (ballonetIndex == 3)
			{//3
				sendMoveMessage(1, 3);
			}
			else 
			if (ballonetIndex == 4)
			{//4
				sendMoveMessage(1, 14);
			}
			else if (ballonetIndex == 5)
			{//2
				sendMoveMessage(1, 12);
			}
			else if (ballonetIndex == 6)
			{//1
				sendMoveMessage(1, 11);
			}
			else if (ballonetIndex == 7)
			{//3
				sendMoveMessage(1, 13);
			}
		}
		else
		{
			if (ballonetIndex == 0)
			{
				sendMoveMessage(1, 9);
			}
			else if (ballonetIndex == 1)
			{
				sendMoveMessage(1, 7);
			}
			else if (ballonetIndex == 2)
			{
				sendMoveMessage(1, 6);
			}
			else if (ballonetIndex == 3)
			{
				sendMoveMessage(1, 8);
			}
			else if (ballonetIndex == 4)
			{
				sendMoveMessage(1, 19);
			}
			else if (ballonetIndex == 5)
			{
				sendMoveMessage(1, 17);
			}
			else if (ballonetIndex == 6)
			{
				sendMoveMessage(1, 16);
			}
			else if (ballonetIndex == 7)
			{
				sendMoveMessage(1, 18);
			}
		}
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
	if ( mOnSetBasePointState[0] && (gGetMsg[23] & 0x01 ) == 0x01 && !mSetBasePointKeyDown[0] )
	{
		mSetBasePointKeyDown[0] = true;
		CSampleGrabberCB *CSgcb1 = SampleGrabberFun(ID_CAMERA1);
		if(0 == CSgcb1->m_bRectifyState)
			CSgcb1->m_bRectifyState = 1;
	}
	else if( (gGetMsg[23] & 0x01 ) == 0x00 )
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
	if ( mOnSetBasePointState[1] && (gGetMsg[23] & 0x40) == 0x40 && !mSetBasePointKeyDown[1] )
	{
		mSetBasePointKeyDown[1] = true;
		CSampleGrabberCB *CSgcb2 = SampleGrabberFun(ID_CAMERA2);
		if(0 == CSgcb2->m_bRectifyState)
			CSgcb2->m_bRectifyState = 1;
	}
	else if ( (gGetMsg[23] & 0x40 ) == 0x00)
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
{//0 1
	BYTE val;
	if( flag )
	{
		val = 0x01;
	}
	else
	{
		val = 0x00;
	}
	m_sendArrTemp7[playerIndex] = val;
}

/////////////////////////////////////////////////////////////////////////
ConsoleFunction( PCVRSubPlayerCoin, void, 3, 3, "sub player coin num" )//ok
{
	if ( gPcvr != NULL )
	{
		gPcvr->subCoinCount(dAtoi(argv[1]), dAtoi(argv[2] ) );
	}
}

//get player coin
ConsoleFunction( PCVRGetPlayerCoin, S32, 1, 1, "get player coin num " )//ok
{
	if ( gPcvr != NULL )
	{
		S32 num = gPcvr->getCoinNum();
		return num;
	}
	else
		return 0;
}

ConsoleFunction( PCVRFlashPlayerStartLight, void, 3, 3, "flash start light by index" )//ok
{
	if ( gPcvr != NULL )
	{
		if (dAtob(argv[2]))
		{
			gPcvr->flashStartLight( dAtoi(argv[1]) + 1, 1 );
		}
		else
		{
			gPcvr->flashStartLight( dAtoi(argv[1]) + 1, 0 );
		}
	}
}

ConsoleFunction( PCVRSetGunShakingState, void, 3, 3, "set the gun shake state")//ok
{
	if ( gPcvr != NULL )
	{
		gPcvr->setPlayerHIDSpeed( dAtoi( argv[1] ) + 1, dAtoi(argv[2] ) );
	}
}

ConsoleFunction( PCVRSetGunShakingLevel, void, 3, 3, "set the gun shake level")//ok
{
	if ( gPcvr != NULL )
	{
		gPcvr->setShakeLevel( dAtoi( argv[1] ) + 1, dAtoi(argv[2] ) );
	}
}

ConsoleFunction( getHIDCoord, const char *, 2, 2, "get hardware coord unchanged" )//
{
	Point2I pos;
	int index = dAtoi( argv[1] );
	if ( index == 1 && gPcvr != NULL )
	{
		pos = gPcvr->m_saveCoord[0];
	}
	else if( index == 2 && gPcvr != NULL)
	{
		pos = gPcvr->m_saveCoord[1];
	}
	char *returnBuffer = Con::getReturnBuffer(100);
	dSprintf(returnBuffer,100,"%d %d ",pos.x,pos.y );
	return returnBuffer;
}

ConsoleFunction( PCVRSetBasePoint, void, 2, 2, "reset base point" )//ok
{
	if ( gPcvr != NULL)
	{
		int index = dAtoi( argv[1] );
		//gPcvr->resetBasePoint( index );
		gPcvr->setBasePoint(index );
	}
}

ConsoleFunction(enablePcvrGun, void, 2, 2, "set pcvr enable or disable" )//脚本中调用--处理
{
	if ( gPcvr != NULL )
	{
		if ( dAtob( argv[1] ) )
		{
			gPcvr->Enable();
		}
		else
		{
			gPcvr->Disable();
		}
	}
	Con::printf("enablePcvrGun was called!");
}

ConsoleFunction( getPlayerFireState, bool, 2, 2, "player Index")// no use -- zai sou
{
	if ( gPcvr != NULL )
	{
		if ( dAtoi(argv[1] ) == 1  )
		{
			return gPcvr->b_playerOneOnFire;
		}
		if ( dAtoi(argv[1] ) == 2 )
		{
			return gPcvr->b_playerTwoOnFire;
		}
	}
}

ConsoleFunction( getPlayerRocketState, bool, 2, 2, "player Index")// no use -- zai sou
{
	if ( gPcvr != NULL )
	{
		if ( dAtoi(argv[1] ) == 1 )
		{
			return gPcvr->b_yaoganUp;
		}
		if ( dAtoi(argv[1] ) == 2 )
		{
			return gPcvr->b_playerTwoOnRocket;
		}
	}
}

//boss light - jin du deng
ConsoleFunction( PCVRSetProcessLightLevel, void, 2, 2, "set boss damage level" )//ok
{
	if( gPcvr != NULL )
	{
		gPcvr->setProcessLightLevel( dAtoi(argv[1] ) );
	}
}

//shan guang deng
ConsoleFunction( PCVROpenFlashLight, void, 2, 2, "open the flash light" )//ok
{
	if( gPcvr != NULL )
	{
		gPcvr->openFlashLight(dAtob(argv[1]));
	}
}

//feng shan
ConsoleFunction( PCVRSetFanRotateState, void, 3, 3, "open or close the fan" )//ok
{
	if ( gPcvr != NULL )
	{
		gPcvr->setFanRotateState( dAtoi(argv[1]), dAtob(argv[2]) );
	}
}

//qi nang
ConsoleFunction( PCVRSetBallonetState, void, 3, 3, "set ballonet state")//ok
{
	if ( gPcvr != NULL )
	{
		gPcvr->setBallonetState( dAtoi( argv[1] ), dAtob(argv[2] ) );
	}
}

//是否需要摇杆功能
ConsoleFunction( PCVRSetRockerState, void, 2, 2, "set whether need rocker" )//ok
{
	if ( gPcvr != NULL )
	{
		gPcvr->setRockerState(dAtob(argv[1]));
	}
}

//选关和签名打开
ConsoleFunction( PCVRSetSendScriptsState, void, 2, 2, "")//ok
{
	if ( gPcvr != NULL )
	{
		gPcvr->setSendScriptsState( dAtob( argv[1] ) );
	}
}

//打开激光器----no use
ConsoleFunction( PCVROpenPlayerGun, void, 3, 3, "open the gun" )
{
	if ( gPcvr != NULL )
	{
		gPcvr->openPlayerGun(dAtoi(argv[1]),dAtob( argv[2] ) );
	}
}

ConsoleFunction( PCVRSetPlayerCrossPos, void, 4, 4, "")// ok
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
