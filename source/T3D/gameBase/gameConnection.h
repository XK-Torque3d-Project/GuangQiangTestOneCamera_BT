//-----------------------------------------------------------------------------
// Torque
// Copyright GarageGames, LLC 2011
//-----------------------------------------------------------------------------

#ifndef _GAMECONNECTION_H_
#define _GAMECONNECTION_H_

#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif
#ifndef _GAMEBASE_H_
#include "T3D/gameBase/gameBase.h"
#endif
#ifndef _NETCONNECTION_H_
#include "sim/netConnection.h"
#endif
#ifndef _MOVEMANAGER_H_
#include "T3D/gameBase/moveManager.h"
#endif
#ifndef _BITVECTOR_H_
#include "core/bitVector.h"
#endif

enum GameConnectionConstants
{
   MaxClients = 126,
   DataBlockQueueCount = 16
};

class SFXProfile;
class MatrixF;
class MatrixF;
class Point3F;
class MoveManager;
class MoveList;
struct Move;
struct AuthInfo;

#define GameString TORQUE_APP_NAME

const F32 MinCameraFov              = 1.f;      ///< min camera FOV
const F32 MaxCameraFov              = 179.f;    ///< max camera FOV

const S32 MaxPlayerNum	= 2;							// 最大玩家数

extern Point3F gMouseWorldPt;							// 鼠标世界坐标
extern Point3F gMouseScreenPt;							// 鼠标屏幕坐标

extern Point3F gMouseWorldPt1;							// 鼠标世界坐标
extern Point3F gMouseScreenPt1;							// 鼠标屏幕坐标

extern RayInfo MouseRayinfo;                                   //鼠标射线

extern RayInfo GunRayinfo_0;
extern RayInfo GunRayinfo_1;
extern RayInfo GunRayinfoAy[2];

extern RayInfo gGunRayinfoF;                                  //枪1瞄准射线
extern RayInfo gGunRayinfoS;                                  //枪2瞄准射线

extern Point3F gMouserayOri;   
extern Point3F gMouserayDir;                                  //瞄准向量

extern Point3F gGunOri_0;                                     //枪瞄准向量
extern Point3F gGunDir_0;
extern Point3F gGunDir_1;
extern Point3F gGunOri_1;
extern Point3F gGunDirAy[2];
extern Point3F gGunOriAy[2];

extern Point3F gPcvrPointW0;							// 外设用枪1
extern Point3F gPcvrPointWAy[2];
extern Point3F gPcvrPointS0;							// 外设用枪1
extern Point3F gPcvrPointSAy[2];						// 外设用枪3-x

extern Point3F gPcvrPointW1;							// 外设用枪2
extern Point3F gPcvrPointS1;							// 外设用枪2

extern Point3F gPcvrPointW01;							// 外设用枪1
extern Point3F gPcvrPointS01;							// 外设用枪1

extern Point3F gPcvrPointW11;							// 外设用枪2
extern Point3F gPcvrPointS11;							// 外设用枪2

extern Point2I pt_currentPot0;
extern Point2I pt_currentPot1;

extern VectorF gGunV0;									// 第一把枪的朝向向量
extern VectorF gGunV1;									// 第二把枪的朝向向量
extern VectorF gGunVAy[2];

extern VectorF gGunP0;									// 第一把枪的位置
extern VectorF gGunP1;									// 第二把枪的位置

extern bool b_gPaused;
extern bool bGameTestSelf;
//采集器冷却时间.
extern float TimeCameraMinFree;
//采集器控制开关的单位时间ms.
extern float TimeCameraMin;

class GameConnection : public NetConnection
{
private:
   typedef NetConnection Parent;

   SimObjectPtr<GameBase> mControlObject;
   SimObjectPtr<GameBase> mCameraObject;
   U32 mDataBlockSequence;
   char mDisconnectReason[256];

   U32  mMissionCRC;             // crc of the current mission file from the server

private:
   U32 mLastControlRequestTime;
   S32 mDataBlockModifiedKey;
   S32 mMaxDataBlockModifiedKey;

   /// @name Client side first/third person
   /// @{

   ///
   bool  mFirstPerson;     ///< Are we currently first person or not.
   bool  mUpdateFirstPerson; ///< Set to notify client or server of first person change.
   bool  mUpdateCameraFov; ///< Set to notify server of camera FOV change.
   F32   mCameraFov;       ///< Current camera fov (in degrees).
   F32   mCameraPos;       ///< Current camera pos (0-1).
   F32   mCameraSpeed;     ///< Camera in/out speed.
   /// @}

public:

   /// @name Protocol Versions
   ///
   /// Protocol versions are used to indicated changes in network traffic.
   /// These could be changes in how any object transmits or processes
   /// network information. You can specify backwards compatibility by
   /// specifying a MinRequireProtocolVersion.  If the client
   /// protocol is >= this min value, the connection is accepted.
   ///
   /// Torque (V12) SDK 1.0 uses protocol  =  1
   ///
   /// Torque SDK 1.1 uses protocol = 2
   /// Torque SDK 1.4 uses protocol = 12
   /// @{
   static const U32 CurrentProtocolVersion;
   static const U32 MinRequiredProtocolVersion;
   /// @}

   /// Configuration
   enum Constants {
      BlockTypeMove = NetConnectionBlockTypeCount,
      GameConnectionBlockTypeCount,
      MaxConnectArgs = 16,
      DataBlocksDone = NumConnectionMessages,
      DataBlocksDownloadDone,
   };

   /// Set connection arguments; these are passed to the server when we connect.
   void setConnectArgs(U32 argc, const char **argv);

   /// Set the server password to use when we join.
   void setJoinPassword(const char *password);

   /// @name Event Handling
   /// @{

   virtual void onTimedOut();
   virtual void onConnectTimedOut();
   virtual void onDisconnect(const char *reason);
   virtual void onConnectionRejected(const char *reason);
   virtual void onConnectionEstablished(bool isInitiator);
   virtual void handleStartupError(const char *errorString);
   /// @}

   /// @name Packet I/O
   /// @{

   virtual void writeConnectRequest(BitStream *stream);
   virtual bool readConnectRequest(BitStream *stream, const char **errorString);
   virtual void writeConnectAccept(BitStream *stream);
   virtual bool readConnectAccept(BitStream *stream, const char **errorString);
   /// @}

   bool canRemoteCreate();

private:
   /// @name Connection State
   /// This data is set with setConnectArgs() and setJoinPassword(), and
   /// sent across the wire when we connect.
   /// @{

   U32      mConnectArgc;
   char *mConnectArgv[MaxConnectArgs];
   char *mJoinPassword;
   /// @}

protected:
   struct GamePacketNotify : public NetConnection::PacketNotify
   {
      S32 cameraFov;
      GamePacketNotify();
   };
   PacketNotify *allocNotify();

   bool mControlForceMismatch;

   Vector<SimDataBlock *> mDataBlockLoadList;

public:

   MoveList *mMoveList;

protected:
   bool        mAIControlled;
   AuthInfo *  mAuthInfo;

   static S32  mLagThresholdMS;
   S32         mLastPacketTime;
   bool        mLagging;

   /// @name Flashing
   ////
   /// Note, these variables are not networked, they are for the local connection only.
   /// @{
   F32 mDamageFlash;
   F32 mWhiteOut;

   F32   mBlackOut;
   S32   mBlackOutTimeMS;
   S32   mBlackOutStartTimeMS;
   bool  mFadeToBlack;

   /// @}

   /// @name Packet I/O
   /// @{

   void readPacket      (BitStream *bstream);
   void writePacket     (BitStream *bstream, PacketNotify *note);
   void packetReceived  (PacketNotify *note);
   void packetDropped   (PacketNotify *note);
   void connectionError (const char *errorString);

   void writeDemoStartBlock   (ResizeBitStream *stream);
   bool readDemoStartBlock    (BitStream *stream);
   void handleRecordedBlock   (U32 type, U32 size, void *data);
   /// @}
   void ghostWriteExtra(NetObject *,BitStream *);
   void ghostReadExtra(NetObject *,BitStream *, bool newGhost);
   void ghostPreRead(NetObject *, bool newGhost);
   
   virtual void onEndGhosting();

public:

   DECLARE_CONOBJECT(GameConnection);
   void handleConnectionMessage(U32 message, U32 sequence, U32 ghostCount);
   void preloadDataBlock(SimDataBlock *block);
   void fileDownloadSegmentComplete();
   void preloadNextDataBlock(bool hadNew);
   
   static void consoleInit();

   void setDisconnectReason(const char *reason);
   GameConnection();
   ~GameConnection();

   bool onAdd();
   void onRemove();

   static GameConnection *getConnectionToServer() 
   { 
      return dynamic_cast<GameConnection*>((NetConnection *) mServerConnection); 
   }
   
   static GameConnection *getLocalClientConnection() 
   { 
      return dynamic_cast<GameConnection*>((NetConnection *) mLocalClientConnection); 
   }

   /// @name Control object
   /// @{

   ///
   void setControlObject(GameBase *);
   GameBase* getControlObject() {  return  mControlObject; }
   const GameBase* getControlObject() const {  return  mControlObject; }
   
   void setCameraObject(GameBase *);
   GameBase* getCameraObject();
   
   bool getControlCameraTransform(F32 dt,MatrixF* mat);
   bool getControlCameraVelocity(Point3F *vel);

   bool getControlCameraDefaultFov(F32 *fov);
   bool getControlCameraFov(F32 *fov);
   bool setControlCameraFov(F32 fov);
   bool isValidControlCameraFov(F32 fov);
   
   // Used by editor
   bool isControlObjectRotDampedCamera();

   void setFirstPerson(bool firstPerson);
   
   /// @}

   void detectLag();

   /// @name Datablock management
   /// @{

   S32  getDataBlockModifiedKey     ()  { return mDataBlockModifiedKey; }
   void setDataBlockModifiedKey     (S32 key)  { mDataBlockModifiedKey = key; }
   S32  getMaxDataBlockModifiedKey  ()  { return mMaxDataBlockModifiedKey; }
   void setMaxDataBlockModifiedKey  (S32 key)  { mMaxDataBlockModifiedKey = key; }

   /// Return the datablock sequence number that this game connection is on.
   /// The datablock sequence number is synchronized to the mission sequence number
   /// on each datablock transmission.
   U32 getDataBlockSequence() { return mDataBlockSequence; }
   
   /// Set the datablock sequence number.
   void setDataBlockSequence(U32 seq) { mDataBlockSequence = seq; }

   /// @}

   /// @name Fade control
   /// @{

   F32 getDamageFlash() const { return mDamageFlash; }
   F32 getWhiteOut() const { return mWhiteOut; }

   void setBlackOut(bool fadeToBlack, S32 timeMS);
   F32  getBlackOut();
   /// @}

   /// @name Authentication
   ///
   /// This is remnant code from Tribes 2.
   /// @{

   void            setAuthInfo(const AuthInfo *info);
   const AuthInfo *getAuthInfo();
   /// @}

   /// @name Sound
   /// @{

   void play2D(SFXProfile *profile);
   void play3D(SFXProfile *profile, const MatrixF *transform);
   /// @}

   /// @name Misc.
   /// @{

   bool isFirstPerson() const  { return mCameraPos == 0; }
   bool isAIControlled() { return mAIControlled; }

   void doneScopingScene();
   void demoPlaybackComplete();

   void setMissionCRC(U32 crc)           { mMissionCRC = crc; }
   U32  getMissionCRC()           { return(mMissionCRC); }
   /// @}

   static Signal<void(F32)> smFovUpdate;
   static Signal<void()> smPlayingDemo;

protected:
   DECLARE_CALLBACK( void, onConnectionTimedOut, () );
   DECLARE_CALLBACK( void, onConnectionAccepted, () );
   DECLARE_CALLBACK( void, onConnectRequestTimedOut, () );
   DECLARE_CALLBACK( void, onConnectionDropped, (const char* reason) );
   DECLARE_CALLBACK( void, onConnectRequestRejected, (const char* reason) );
   DECLARE_CALLBACK( void, onConnectionError, (const char* errorString) );
   DECLARE_CALLBACK( void, onDrop, (const char* disconnectReason) );
   DECLARE_CALLBACK( void, initialControlSet, () );
   DECLARE_CALLBACK( void, onControlObjectChange, () );
   DECLARE_CALLBACK( void, setLagIcon, (bool state) );
   DECLARE_CALLBACK( void, onDataBlocksDone, (U32 sequence) );
   DECLARE_CALLBACK( void, onFlash, (bool state) );
};

#endif
