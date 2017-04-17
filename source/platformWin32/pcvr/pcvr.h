//动感-串口

#pragma once

#include "gui/core/guiCanvas.h"
#include <windows.h>

class pcvr;
extern pcvr* gPcvr;	// 枪硬件库指针。

class pcvr
{
public:

	enum JIAOYANENUM
	{
		EMPTY = 0,
		SUCCEED = 1,
		FAILED = 2,
	};

	pcvr(void);
	~pcvr(void);

	bool	createGun();						//建立枪通讯
	bool	deleteGun();						//释放枪对象

	bool	Init();
	void	Enable();
	void	Disable();

	bool	openDevice();
	void	subCoinCount( int playerIndex, int count );
	int		getCoinNum();
	void    setPlayerHIDSpeed( int playerIndex, int speedIndex );
	void    setShakeLevel( U32 playerIndex, U32 index );
	void	sendMoveMessage( int pIndex, int messageIndex );
	void	flashStartLight( int index, int startIndex );
	void    resetBasePoint( int index );
	void	sendMessage(bool flag);
	void	setSendScriptsState(bool isSend);
	void	setRockerState( bool state );
	void	setProcessLightLevel( int level );
	void	openFlashLight( bool flash );
	void	setFanRotateState( S32 playerIndex, bool state );
	void	setBallonetState( int ballonetIndex, bool val );
	void	openPlayerGun( int playerIndex, bool flag );

	void	createCamera();
	void	releaseCamera();
	void	reSetCamera();
	void    enableCamera( bool );
	void	setCamPointCallBackFun();
	void	setBasePoint( int playerIndex );
	void	camSetBasePoint();

	void	keyProcess();
	GuiCanvas *rootCanvas;

	void	RandomJiaoYanDt();
	void	StartJiaoYanIO();
	void	OnEndJiaoYanIO(JIAOYANENUM val);
	void	InitJiaoYanMiMa();
	void	RandomJiaoYanMiMaVal();
	void SetjiaoyanResult(bool falg);

public:
	// Initial conditions
	enum PcvrConstants {
		BufGetLen = 39,
		BufSendLen = 32,
	};	

public:
	bool b_enable;
	bool b_HasCreateGun;
	int enableNum;
	BYTE m_sendArray[BufSendLen];
	BYTE m_sendArrTemp5[8];
	BYTE m_sendArrTemp6[8];
	BYTE m_sendArrTemp7[8];
	/************************************************************************/
	/*m_JiGuangQiState[x] == 0x00 -> 激光器关闭                             */
	/*m_JiGuangQiState[x] == 0x01 -> 激光器打开                             */
	/************************************************************************/
	BYTE m_JiGuangQiState[8];
	Point2I m_saveCoord[2];
	BYTE  gGetMsg[BufGetLen];
	BYTE  gSendMsg[BufSendLen];

	int m_disConnectTime;
	int m_tempCount;

	S32 oldHitCtrl0Id;
	S32 oldHitCtrl1Id;

	bool isMouse1Up;
	bool isMouse2Up;
	bool b_setEnterKeyDown;
	bool b_setMoveKeyDown;
	bool b_playerOneOnFire;
	bool b_playerTwoOnFire;
	bool b_playerOneStartKeyDown;
	bool b_playerTwoStartKeyDown;
	bool b_jiaotabanOneDown;
	bool b_jiaotabanTwoDown;
	bool b_playerOneOnRocket;
	bool b_playerTwoOnRocket;
	bool b_yaoganUp;
	bool b_yaoganDown;
	bool b_yaoganLeft;
	bool b_yaoganRight;

	bool b_BeifenAnjian1;
	bool b_BeifenAnjian2;
	bool b_BeifenAnjian3;
	bool b_BeifenAnjian4;
	bool b_BeifenAnjian5;
	bool b_BeifenAnjian6;

	bool isShanguangdeng;
	int shanJiangeTime;

	Point2I m_equal;

	Point2F pt_one_leftTop;
	Point2F pt_one_rightTop;
	Point2F pt_one_rightBottom;
	Point2F pt_one_leftBottom;

	Point2F pt_two_leftTop;
	Point2F pt_two_rightTop;
	Point2F pt_two_rightBottom;
	Point2F pt_two_leftBottom;

	bool b_playerShakeP1;
	bool b_playerShakeP2;

	bool IsCleanHidCoin;
	bool IsCleanHidCoinP2;
	char coinStr[10];

	bool    mSendHidMsgToScripts;
	bool	mNeedRocker;
	BYTE	BossLightValue;

	bool	mCameraEnable;
	bool	mOnSetBasePointState[2];
	bool	mSetBasePointKeyDown[2];
};
