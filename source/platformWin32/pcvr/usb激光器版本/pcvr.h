#pragma once

#include "gui/core/guiCanvas.h"
#include <windows.h>

#include "SLABHIDDevice.h"

#ifndef HID_BUFFER_LEN
	#define HID_BUFFER_LEN 63
#endif


class pcvr;
extern pcvr* gPcvr;	// 枪硬件库指针。

typedef void (WINAPI* ashPINTPROC)(int,POINT);  

class pcvr
{
public:
	pcvr(void);
	~pcvr(void);

	bool	createGun();						//建立枪通讯
	bool	deleteGun();						//释放枪对象
	bool	Init();
	bool	openDevice();
	void	sendMessage();
	bool	getMessage();
	void	keyProcess();
	void    setSendMsg( int index, BYTE val );
	void	cleanSendMsg();
	void	printArray( BYTE *array, int arraySize );

	void	subPlayerCoin( int playerIndex, int coinNum );
	S32		getCoinNum();
	void	flashPlayerStartLight( int playerIndex, bool flash );
	void	setShakeLevel( int playerIndex, int val );
	void	setBallonetState( int ballonetIndex, bool val );
	void	setProcessLightLevel( int level );
	void	setGunShakeState( int playerIndex, int state );

	void	createCamera();
	void	releaseCamera();
	void	reSetCamera();
	void    enableCamera( bool );
	void	setCamPointCallBackFun();
	void	setBasePoint( int playerIndex );
	void	camSetBasePoint();
	void	openPlayerGun( int playerIndex, bool flag );
	void	setRockerState( bool state );
	void	openFlashLight( bool flash );
	void	setFanRotateState( S32 playerIndex, bool state );
	
	void	setSendScriptsState( bool isSend );
	void	setRandomValue(byte *buffer);

public:

	BYTE	mSendArray[HID_BUFFER_LEN];
	BYTE    mGetArray[HID_BUFFER_LEN];
	
	bool	mPlayerShotMsg[4];
	bool	mPlayerStartMsg[4];
	bool	mRocketShotMsg[4];
	bool	mPedalAniMsg[2];
	bool	mSetPlaneMsg[2];
	bool	mOnSetBasePointState[2];
	bool	mSetBasePointKeyDown[2];
	BYTE	mPlayerCoinMsg[4];
	BYTE	mPlayerCoordMsg[4][4];
	BYTE	mPlayerShakeGunMsg[4];
	BYTE	mRockerRot[4];

	bool	mCameraEnable;
	bool    mSendHidMsgToScripts;
	HID_DEVICE	gTestHidPtr;
	bool	mNeedRocker;

	bool	mPlayerOpenGunMsg[4];
	bool	mPlayerCanSubCoin[2];
	S32		mPlayerSubCoinNum[2];
};