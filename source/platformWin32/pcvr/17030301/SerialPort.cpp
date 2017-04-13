//#include "StdAfx.h"
#include "SerialPort.h"
#include <process.h>
#include <iostream>
#include "pcvr.h"
#include "T3D/gameBase/gameConnection.h"

/** 线程退出标志 */ 
bool CSerialPort::s_bExit = false;
/** 当串口无数据时,sleep至下次查询间隔的时间,单位:秒 */ 
const UINT SLEEP_TIME_INTERVAL = 5;

const int BufGetLen = 39;
const int BufSendLen = 32;
const byte ReadHEAD1 = 0x01;
const byte ReadHEAD2 = 0x55;
const byte ReadEND1 = 0x41;
const byte ReadEND2 = 0x42;
const byte ReadEND3 = 0x43;
const byte ReadEND4 = 0x44;
byte middleTemp = 0x00;
byte tempRecord[BufGetLen];
bool success21 = false;
bool recordFail = true;
int firsTongxin = 0;
int countABC = 0;

CSerialPort::CSerialPort(void)
: m_hListenThread(INVALID_HANDLE_VALUE)
{
	m_hComm = INVALID_HANDLE_VALUE;
	m_hListenThread = INVALID_HANDLE_VALUE;

	InitializeCriticalSection(&m_csCommunicationSync);

}

CSerialPort::~CSerialPort(void)
{Con::printf("111112222cloos");
	CloseListenTread();
	ClosePort();
	DeleteCriticalSection(&m_csCommunicationSync);
}

bool CSerialPort::InitPort( UINT portNo /*= 1*/,UINT baud /*= CBR_9600*/,char parity /*= 'N'*/,
						    UINT databits /*= 8*/, UINT stopsbits /*= 1*/,DWORD dwCommEvents /*= EV_RXCHAR*/ )
{
	ZeroMemory( tempRecord, sizeof( tempRecord ) );

	/** 临时变量,将制定参数转化为字符串形式,以构造DCB结构 */ 
	char szDCBparam[50];
	sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);

	/** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */ 
	if (!openPort(portNo))
	{
		return false;
	}

	/** 进入临界段 */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** 是否有错误发生 */ 
	BOOL bIsSuccess = TRUE;

    /** 在此可以设置输入输出的缓冲区大小,如果不设置,则系统会设置默认值.
	 *  自己设置缓冲区大小时,要注意设置稍大一些,避免缓冲区溢出
	 */
	/*if (bIsSuccess )
	{
		bIsSuccess = SetupComm(m_hComm,10,10);
	}*/

	/** 设置串口的超时时间,均设为0,表示不使用超时限制 */
	COMMTIMEOUTS  CommTimeouts;
	CommTimeouts.ReadIntervalTimeout         = 0;
	CommTimeouts.ReadTotalTimeoutMultiplier  = 0;
	CommTimeouts.ReadTotalTimeoutConstant    = 0;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	CommTimeouts.WriteTotalTimeoutConstant   = 0; 
	if ( bIsSuccess)
	{
		bIsSuccess = SetCommTimeouts(m_hComm, &CommTimeouts);
	}

	DCB  dcb;
	if ( bIsSuccess )
	{
		// 将ANSI字符串转换为UNICODE字符串
		DWORD dwNum = MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, NULL, 0);
		wchar_t *pwText = new wchar_t[dwNum] ;
		if (!MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, pwText, dwNum))
		{
			bIsSuccess = TRUE;
		}

		/** 获取当前串口配置参数,并且构造串口DCB参数 */ 
		bIsSuccess = GetCommState(m_hComm, &dcb) && BuildCommDCB(pwText, &dcb) ;
		/** 开启RTS flow控制 */ 
		dcb.fRtsControl = RTS_CONTROL_ENABLE; 

		/** 释放内存空间 */ 
		delete [] pwText;
	}

	if ( bIsSuccess )
	{
		/** 使用DCB参数配置串口状态 */ 
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}
		
	/**  清空串口缓冲区 */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** 离开临界段 */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return bIsSuccess==TRUE;
}

bool CSerialPort::InitPort( UINT portNo ,const LPDCB& plDCB )
{
	/** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */ 
	if (!openPort(portNo))
	{
		return false;
	}
	
	/** 进入临界段 */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** 配置串口参数 */ 
	if (!SetCommState(m_hComm, plDCB))
	{
		return false;
	}

	/**  清空串口缓冲区 */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** 离开临界段 */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

void CSerialPort::ClosePort()
{
	/** 如果有串口被打开，关闭它 */
	if( m_hComm != INVALID_HANDLE_VALUE )
	{
		CloseHandle( m_hComm );
		m_hComm = INVALID_HANDLE_VALUE;
	}
}

bool CSerialPort::openPort( UINT portNo )
{
	/** 进入临界段 */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** 把串口的编号转换为设备名 */ 
    char szPort[50];
	sprintf_s(szPort, "COM%d", portNo);

	/** 打开指定的串口 */ 
	m_hComm = CreateFileA(szPort,		                /** 设备名,COM1,COM2等 */ 
						 GENERIC_READ | GENERIC_WRITE,  /** 访问模式,可同时读写 */   
						 0,                             /** 共享模式,0表示不共享 */ 
					     NULL,							/** 安全性设置,一般使用NULL */ 
					     OPEN_EXISTING,					/** 该参数表示设备必须存在,否则创建失败 */ 
						 0,    
						 0);    

	/** 如果打开失败，释放资源并返回 */ 
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}

	/** 退出临界区 */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

bool CSerialPort::OpenListenThread()
{
	/** 检测线程是否已经开启了 */ 
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** 线程已经开启 */ 
		return false;
	}

	s_bExit = false;
	/** 线程ID */ 
	UINT threadId;
	/** 开启串口数据监听线程 */ 
	m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, &threadId);
	if (!m_hListenThread)
	{
		return false;
	}
	/** 设置线程的优先级,高于普通线程 */ 
	if (!SetThreadPriority(m_hListenThread, THREAD_PRIORITY_ABOVE_NORMAL))
	{
		return false;
	}

	return true;
}

bool CSerialPort::CloseListenTread()
{	Con::printf("111112222CloseListenTread");
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{Con::printf("111112222CloseListenTread");
		/** 通知线程退出 */ 
		s_bExit = true;

		/** 等待线程退出 */ 
		Sleep(10);

		/** 置线程句柄无效 */ 
		CloseHandle( m_hListenThread );
		m_hListenThread = INVALID_HANDLE_VALUE;
	}
	return true;
}

UINT CSerialPort::GetBytesInCOM()
{
	DWORD dwError = 0;	/** 错误码 */ 
	COMSTAT  comstat;   /** COMSTAT结构体,记录通信设备的状态信息 */ 
	memset(&comstat, 0, sizeof(COMSTAT));

	UINT BytesInQue = 0;
	/** 在调用ReadFile和WriteFile之前,通过本函数清除以前遗留的错误标志 */ 
	if ( ClearCommError(m_hComm, &dwError, &comstat) )
	{
		BytesInQue = comstat.cbInQue; /** 获取在输入缓冲区中的字节数 */ 
	}

	return BytesInQue;
}

UINT WINAPI CSerialPort::ListenThread( void* pParam )
{
	/** 得到本类的指针 */ 
	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);

	int countRead = 0;
	// 线程循环,轮询方式读取串口数据
	while (!pSerialPort->s_bExit) 
	{
		if (firsTongxin == 0 && gPcvr->enableNum == 1)
		{
			firsTongxin = 1;
		}
		else if (firsTongxin == 1 && gPcvr->enableNum == 2)
		{
			firsTongxin = 2;
		}
		else if(firsTongxin == 2)
		{
			firsTongxin = 3;
			Sleep(500);
			continue;
		
		}
		
		if (firsTongxin < 3) continue;

		Sleep(20);

		gPcvr->sendMessage(success21);
		pSerialPort->WriteByte(gPcvr->m_sendArray, BufSendLen);	//chang fffffffffff lxy

		UINT BytesInQue = pSerialPort->GetBytesInCOM();

		/** 如果串口输入缓冲区中无数据,则休息一会再查询 */ 
		if ( BytesInQue == 0 || !firsTongxin)
		{
			Sleep(SLEEP_TIME_INTERVAL);
			continue;
		}
		
		
		/** 读取输入缓冲区中的数据并输出显示 */
		byte bRecved = 0x00;
		bool bRecord = false;

		do
		{//lxy fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff get
			bRecved = 0x00;
			if(pSerialPort->ReadByte(bRecved) == true)
			{
				if (countRead == 0 && bRecved != ReadHEAD1)	//head1
				{
					bRecord = false;
					ZeroMemory( tempRecord, sizeof( tempRecord ) );
					Con::printf("%x***n1***%x ", bRecved, ReadHEAD1);
					recordFail = true;
					Con::executef( "OnConnectState", "2" );
					continue;
				}
				else if (countRead == 1 && bRecved != ReadHEAD2)	//head2
				{
					//重新计数
					countRead = 0;
					bRecord = false;
					ZeroMemory( tempRecord, sizeof( tempRecord ) );
					Con::printf("%x***n2***%x ", bRecved, ReadHEAD2);
					recordFail = true;
					Con::executef( "OnConnectState", "2" );
					continue;
				}
				else if ((countRead == BufGetLen - 4) && bRecved != ReadEND1)	//end1
				{
					//重新计数
					countRead = 0;
					bRecord = false;
					ZeroMemory( tempRecord, sizeof( tempRecord ) );
					Con::printf("%x***n3***%x ", bRecved, ReadEND1);
					recordFail = true;
					Con::executef( "OnConnectState", "2" );
					continue;
				}
				else if ((countRead == BufGetLen - 3) && bRecved != ReadEND2)	//end2
				{
					//重新计数
					countRead = 0;
					bRecord = false;
					ZeroMemory( tempRecord, sizeof( tempRecord ) );
					Con::printf("%x***n4***%x ", bRecved, ReadEND2);
					recordFail = true;
					Con::executef( "OnConnectState", "2" );
					continue;
				}
				else if ((countRead == BufGetLen - 2) && bRecved != ReadEND3)	//end2
				{
					//重新计数
					countRead = 0;
					bRecord = false;
					ZeroMemory( tempRecord, sizeof( tempRecord ) );
					Con::printf("%x***n5***%x ", bRecved, ReadEND3);
					recordFail = true;
					Con::executef( "OnConnectState", "2" );
					continue;
				}
				else if ((countRead == BufGetLen - 1) && bRecved != ReadEND4)	//end2
				{
					//重新计数
					countRead = 0;
					bRecord = false;
					ZeroMemory( tempRecord, sizeof( tempRecord ) );
					Con::printf("%x***n6***%x ", bRecved, ReadEND4);
					recordFail = true;
					Con::executef( "OnConnectState", "2" );
					continue;
				}

				tempRecord[countRead] = bRecved;
				bRecord = true;

				countRead++;

				if (countRead >= BufGetLen)
				{
					//校验位33 --- 不包括字头(0 1)字尾(25 26)币值(18)
					middleTemp = 0x00;

					for (int i=2; i<BufGetLen - 4; i++)
					{
						if (i == 18 || i == 33)//change here ............................................
						{
							continue;
						}

						middleTemp ^= tempRecord[i];
					}

					middleTemp ^= ReadHEAD1;
					middleTemp ^= ReadHEAD2;

					middleTemp ^= ReadEND1;
					middleTemp ^= ReadEND2;
					middleTemp ^= ReadEND3;
					middleTemp ^= ReadEND4;

					if (tempRecord[33] != middleTemp)
					{
						//重新计数
						countRead = 0;
						bRecord = false;
						success21 = false;
						ZeroMemory( tempRecord, sizeof( tempRecord ) );
						Con::printf("%x***n2111***%x ", bRecved, ReadEND4);
						recordFail = true;
						Con::executef( "OnConnectState", "2" );
						continue;

					}
					else
					{
						//21 success
						success21 = true;
					}

					countRead = 0;
					break;
				}
				continue;
			}
		}while(--BytesInQue);

		if (countRead == 0 && bRecord) {
			CopyMemory( gPcvr->gGetMsg, tempRecord, sizeof( gPcvr->gGetMsg ) );
			char testGetBuf[4096];
			sprintf(testGetBuf, "%x %x %x %x %x %x %x %x %x %x",
								gPcvr->gGetMsg[0], gPcvr->gGetMsg[1], gPcvr->gGetMsg[2], gPcvr->gGetMsg[3], gPcvr->gGetMsg[4], gPcvr->gGetMsg[5], gPcvr->gGetMsg[6], gPcvr->gGetMsg[7], gPcvr->gGetMsg[8], gPcvr->gGetMsg[9]);
			//Con::printf("GetBuff: %s", testGetBuf);
			sprintf(testGetBuf, "%x %x %x %x %x %x %x %x %x %x",
				gPcvr->gGetMsg[10], gPcvr->gGetMsg[11], gPcvr->gGetMsg[12], gPcvr->gGetMsg[13], gPcvr->gGetMsg[14], gPcvr->gGetMsg[15], gPcvr->gGetMsg[16], gPcvr->gGetMsg[17], gPcvr->gGetMsg[18], gPcvr->gGetMsg[19]);
			//Con::printf("GetBuf: %s", testGetBuf);
			sprintf(testGetBuf, "%x %x %x %x %x %x %x",
				gPcvr->gGetMsg[20], gPcvr->gGetMsg[21], gPcvr->gGetMsg[22], gPcvr->gGetMsg[23], gPcvr->gGetMsg[24], gPcvr->gGetMsg[25], gPcvr->gGetMsg[26]);
			//Con::printf("GetBuf: %s", testGetBuf);
			//Con::printf("GetBuf: %s", testGetBuf);
			//Con::printf("******");
			
			//在加载完成之后开始调用process函数
			gPcvr->Process();
			gPcvr->keyProcess();

			if (recordFail)
			{
				recordFail = false;
				Con::executef( "OnConnectState", "1" );
			}
		}
	}
	return 0;
}

bool CSerialPort::ReadByte( byte &bRecved )
{
	BOOL  bResult     = TRUE;
	DWORD BytesRead   = 0;
	if(m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** 临界区保护 */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** 从缓冲区读取一个字节的数据 */ 
	bResult = ReadFile(m_hComm, &bRecved, 1, &BytesRead, NULL);
	if ((!bResult))
	{ 
		/** 获取错误码,可以根据该错误码查出错误原因 */ 
		//DWORD dwError = GetLastError();

		/** 清空串口缓冲区 */ 
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** 离开临界区 */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return (BytesRead == 1);
}

bool CSerialPort::WriteByte(byte pData[], int length)
{
	BOOL   bResult     = TRUE;
	DWORD  BytesToSend = 0;
	if(m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	/** 临界区保护 */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** 向缓冲区写入指定量的数据 */ 
	bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);
	if (!bResult)  
	{
		//DWORD dwError = GetLastError();
		/** 清空串口缓冲区 */ 
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** 离开临界区 */ 
	LeaveCriticalSection(&m_csCommunicationSync);
	return true;
}
