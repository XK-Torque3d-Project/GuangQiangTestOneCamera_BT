//#include "StdAfx.h"
#include "SerialPort.h"
#include <process.h>
#include <iostream>
#include "pcvr.h"
#include "T3D/gameBase/gameConnection.h"

/** �߳��˳���־ */ 
bool CSerialPort::s_bExit = false;
/** ������������ʱ,sleep���´β�ѯ�����ʱ��,��λ:�� */ 
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

	/** ��ʱ����,���ƶ�����ת��Ϊ�ַ�����ʽ,�Թ���DCB�ṹ */ 
	char szDCBparam[50];
	sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);

	/** ��ָ������,�ú����ڲ��Ѿ����ٽ�������,�����벻Ҫ�ӱ��� */ 
	if (!openPort(portNo))
	{
		return false;
	}

	/** �����ٽ�� */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** �Ƿ��д����� */ 
	BOOL bIsSuccess = TRUE;

    /** �ڴ˿���������������Ļ�������С,���������,��ϵͳ������Ĭ��ֵ.
	 *  �Լ����û�������Сʱ,Ҫע�������Դ�һЩ,���⻺�������
	 */
	/*if (bIsSuccess )
	{
		bIsSuccess = SetupComm(m_hComm,10,10);
	}*/

	/** ���ô��ڵĳ�ʱʱ��,����Ϊ0,��ʾ��ʹ�ó�ʱ���� */
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
		// ��ANSI�ַ���ת��ΪUNICODE�ַ���
		DWORD dwNum = MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, NULL, 0);
		wchar_t *pwText = new wchar_t[dwNum] ;
		if (!MultiByteToWideChar (CP_ACP, 0, szDCBparam, -1, pwText, dwNum))
		{
			bIsSuccess = TRUE;
		}

		/** ��ȡ��ǰ�������ò���,���ҹ��촮��DCB���� */ 
		bIsSuccess = GetCommState(m_hComm, &dcb) && BuildCommDCB(pwText, &dcb) ;
		/** ����RTS flow���� */ 
		dcb.fRtsControl = RTS_CONTROL_ENABLE; 

		/** �ͷ��ڴ�ռ� */ 
		delete [] pwText;
	}

	if ( bIsSuccess )
	{
		/** ʹ��DCB�������ô���״̬ */ 
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}
		
	/**  ��մ��ڻ����� */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** �뿪�ٽ�� */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return bIsSuccess==TRUE;
}

bool CSerialPort::InitPort( UINT portNo ,const LPDCB& plDCB )
{
	/** ��ָ������,�ú����ڲ��Ѿ����ٽ�������,�����벻Ҫ�ӱ��� */ 
	if (!openPort(portNo))
	{
		return false;
	}
	
	/** �����ٽ�� */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** ���ô��ڲ��� */ 
	if (!SetCommState(m_hComm, plDCB))
	{
		return false;
	}

	/**  ��մ��ڻ����� */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	/** �뿪�ٽ�� */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

void CSerialPort::ClosePort()
{
	/** ����д��ڱ��򿪣��ر��� */
	if( m_hComm != INVALID_HANDLE_VALUE )
	{
		CloseHandle( m_hComm );
		m_hComm = INVALID_HANDLE_VALUE;
	}
}

bool CSerialPort::openPort( UINT portNo )
{
	/** �����ٽ�� */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** �Ѵ��ڵı��ת��Ϊ�豸�� */ 
    char szPort[50];
	sprintf_s(szPort, "COM%d", portNo);

	/** ��ָ���Ĵ��� */ 
	m_hComm = CreateFileA(szPort,		                /** �豸��,COM1,COM2�� */ 
						 GENERIC_READ | GENERIC_WRITE,  /** ����ģʽ,��ͬʱ��д */   
						 0,                             /** ����ģʽ,0��ʾ������ */ 
					     NULL,							/** ��ȫ������,һ��ʹ��NULL */ 
					     OPEN_EXISTING,					/** �ò�����ʾ�豸�������,���򴴽�ʧ�� */ 
						 0,    
						 0);    

	/** �����ʧ�ܣ��ͷ���Դ������ */ 
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}

	/** �˳��ٽ��� */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}

bool CSerialPort::OpenListenThread()
{
	/** ����߳��Ƿ��Ѿ������� */ 
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** �߳��Ѿ����� */ 
		return false;
	}

	s_bExit = false;
	/** �߳�ID */ 
	UINT threadId;
	/** �����������ݼ����߳� */ 
	m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, &threadId);
	if (!m_hListenThread)
	{
		return false;
	}
	/** �����̵߳����ȼ�,������ͨ�߳� */ 
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
		/** ֪ͨ�߳��˳� */ 
		s_bExit = true;

		/** �ȴ��߳��˳� */ 
		Sleep(10);

		/** ���߳̾����Ч */ 
		CloseHandle( m_hListenThread );
		m_hListenThread = INVALID_HANDLE_VALUE;
	}
	return true;
}

UINT CSerialPort::GetBytesInCOM()
{
	DWORD dwError = 0;	/** ������ */ 
	COMSTAT  comstat;   /** COMSTAT�ṹ��,��¼ͨ���豸��״̬��Ϣ */ 
	memset(&comstat, 0, sizeof(COMSTAT));

	UINT BytesInQue = 0;
	/** �ڵ���ReadFile��WriteFile֮ǰ,ͨ�������������ǰ�����Ĵ����־ */ 
	if ( ClearCommError(m_hComm, &dwError, &comstat) )
	{
		BytesInQue = comstat.cbInQue; /** ��ȡ�����뻺�����е��ֽ��� */ 
	}

	return BytesInQue;
}

UINT WINAPI CSerialPort::ListenThread( void* pParam )
{
	/** �õ������ָ�� */ 
	CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);

	int countRead = 0;
	// �߳�ѭ��,��ѯ��ʽ��ȡ��������
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

		/** ����������뻺������������,����Ϣһ���ٲ�ѯ */ 
		if ( BytesInQue == 0 || !firsTongxin)
		{
			Sleep(SLEEP_TIME_INTERVAL);
			continue;
		}
		
		
		/** ��ȡ���뻺�����е����ݲ������ʾ */
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
					//���¼���
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
					//���¼���
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
					//���¼���
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
					//���¼���
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
					//���¼���
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
					//У��λ33 --- ��������ͷ(0 1)��β(25 26)��ֵ(18)
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
						//���¼���
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
			
			//�ڼ������֮��ʼ����process����
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

	/** �ٽ������� */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** �ӻ�������ȡһ���ֽڵ����� */ 
	bResult = ReadFile(m_hComm, &bRecved, 1, &BytesRead, NULL);
	if ((!bResult))
	{ 
		/** ��ȡ������,���Ը��ݸô�����������ԭ�� */ 
		//DWORD dwError = GetLastError();

		/** ��մ��ڻ����� */ 
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** �뿪�ٽ��� */ 
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

	/** �ٽ������� */ 
	EnterCriticalSection(&m_csCommunicationSync);

	/** �򻺳���д��ָ���������� */ 
	bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);
	if (!bResult)  
	{
		//DWORD dwError = GetLastError();
		/** ��մ��ڻ����� */ 
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** �뿪�ٽ��� */ 
	LeaveCriticalSection(&m_csCommunicationSync);
	return true;
}
