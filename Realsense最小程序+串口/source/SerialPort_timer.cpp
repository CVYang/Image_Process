//////////////////////////////////////////////////////////////////////////
/// COPYRIGHT NOTICE
/// Copyright (c) 2009, ���пƼ���ѧtickTick Group  ����Ȩ������
/// All rights reserved.
/// 
/// @file    SerialPort.h  
/// @brief   ����ͨ����ͷ�ļ�
///
/// ���ļ���ɴ���ͨ���������
///
/// @version 1.0   
/// @author  ¬�� 
/// @E-mail��lujun.hust@gmail.com
/// @date    2010/03/19
///
///  �޶�˵����
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SerialPort_timer.h"
#include <process.h>
#include <iostream>
#include "Mmsystem.h"

/** �߳��˳���־ */ 
bool CSerialPort::s_bExit = false;
/** ������������ʱ,sleep���´β�ѯ�����ʱ��,��λ:�� */ 
const UINT SLEEP_TIME_INTERVAL = 5;

CSerialPort::CSerialPort(void)
: m_hListenThread(INVALID_HANDLE_VALUE)
{
	m_hComm = INVALID_HANDLE_VALUE;
	m_hListenThread = INVALID_HANDLE_VALUE;

	InitializeCriticalSection(&m_csCommunicationSync);

}

CSerialPort::~CSerialPort(void)
{
	CloseListenTread();
	ClosePort();
	DeleteCriticalSection(&m_csCommunicationSync);
}

bool CSerialPort::InitPort( UINT portNo /*= 1*/,UINT baud /*= CBR_115200*/,char parity /*= 'N'*/,
						    UINT databits /*= 8*/, UINT stopsbits /*= 1*/,DWORD dwCommEvents /*= EV_RXCHAR*/ )
{
	/** ��ʱ����,���ƶ�����ת��Ϊ�ַ�����ʽ,�Թ���DCB�ṹ */ 
	char szDCBparam[50];
	//sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);
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
	if (bIsSuccess )
	{
		bIsSuccess = SetupComm(m_hComm,50,50);
	}

	/** ���ô��ڵĳ�ʱʱ��,����Ϊ0,��ʾ��ʹ�ó�ʱ���� */
	COMMTIMEOUTS  CommTimeouts;
	CommTimeouts.ReadIntervalTimeout              = 0;
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
	m_hComm = CreateFileA(szPort,		                  /** �豸��,COM1,COM2�� */ 
						 GENERIC_READ | GENERIC_WRITE,  /** ����ģʽ,��ͬʱ��д */   
						 0,                                                      /** ����ģʽ,0��ʾ������ */ 
					     NULL,					                       		  /** ��ȫ������,һ��ʹ��NULL */ 
					     OPEN_EXISTING,				              /** �ò�����ʾ�豸�������,���򴴽�ʧ�� */ 
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

/**************************************���������ò�������Ҫ������Ҫextern********************************************/
//����
int  DATA_NUM  = 6;//8λ����������
uchar BAOTOU = 0x33, BAOWEI1 = 0x44, BAOWEI2 = 0x55;
//���ݱ���
char cRecved = 0x00;
uchar date_receive[10] = { 0 };
int data_index = 0;
int baotou_flag=0;
int usart_get_time_det,usart_get_lst;//���βɼ�ʱ���
int usart_get_time_lst,usart_get_time_llst,usart_get_time_this;//�˴Σ��ϴΣ����ϴ�ʱ��
//extern
extern int  usart_rt_flag;
extern uchar Flag_Rec_1, Flag_Rec_2, Flag_Rec_3;
extern CSerialPort mySerialPort;

/****************************************�������ڽ��ն�ʱ���ж϶�ʱ���ж�********************************************/
//void WINAPI TimerProc(HWND hWnd,UINT nMsg,UINT nTimerid,DWORD dwTime)
void PASCAL timer_f(UINT wTimerID, UINT msg,DWORD dwUser,DWORD dwl,DWORD dw2)
{
	//��¼ϵͳʱ��
	//SYSTEMTIME time1;
	//GetSystemTime(&time1);
	//int min,ms,second;
	//min = time1.wMinute;
	//second = time1.wSecond;
	//ms = time1.wMilliseconds;
	//��ӡϵͳʱ��
	//fprintf(coordinate_save,"%d\t%d\n",min*60*1000 + second*1000 + ms,data_index);

	 /** �õ������ָ�� */ 
	//CSerialPort *pSerialPort = reinterpret_cast<CSerialPort*>(pParam);
	//�߳�ѭ��,��ѯ��ʽ��ȡ��������
	//��ѯ�������ж��ٸ��ֽڵ�����

	UINT BytesInQue = mySerialPort.GetBytesInCOM();

	if ( BytesInQue == 0 )
	{
		//����������뻺������������,����Ϣһ���ٲ�ѯ
		//Sleep(SLEEP_TIME_INTERVAL);
	}
	else 
	{
		//��ȡ���뻺�����е����ݲ������ʾ 
		/**********************Do-While��ѭ��********************/
		do
		{
			cRecved = 0x00;
			/**********************�յ����ݽ��д���********************/
			//Ӧ����˳���ȡ
			if(mySerialPort.ReadChar(cRecved) == true)
			{
				//��ͷУ�飬��һ�ε�����
				if (!baotou_flag)
				{	
					if ((uchar)cRecved == BAOTOU)
						{
							baotou_flag=1;	
						}
					data_index=0;
				}
				//��βУ�飬ͬʱ����������ȡ���
				else if ((uchar)cRecved == BAOWEI2)
				{
					//����յ���ȷ��β
					if (date_receive[data_index - 1] == BAOWEI1)
					{
					
						///////Ŀǰû�б�Ҫ////////
						//��������������ͬʱ�ں���֮�н��У���ѭ���������
						//while(usart_rt_flag);
						//�ָ�׶ο�ʼ��־
						usart_rt_flag=1;

						////�ռ����ݽ��д�����ֵ
						Flag_Rec_1 = date_receive[0];
						Flag_Rec_2 = date_receive[1];
						Flag_Rec_3 = date_receive[2];

						/*cout << "���Գɹ�" << endl;*/
						/////////��¼ʱ��/////////
						SYSTEMTIME time1;
						GetSystemTime(&time1);
						int min,ms,second;
						min = time1.wMinute;
						second = time1.wSecond;
						ms = time1.wMilliseconds;
						usart_get_time_llst = usart_get_time_lst;
						usart_get_time_lst = usart_get_time_this;
						usart_get_time_this = min * 60 * 1000 + second * 1000 + ms;
						//����ʱ���
						usart_get_lst = usart_get_time_det;
						usart_get_time_det = usart_get_time_this - usart_get_time_lst;

						//flag����
						baotou_flag=0;
						data_index=0;
						//�ָ�׶ν���
						usart_rt_flag=0;
					}
					//δ�ﵽ��β
					else 
					{
						date_receive[data_index]=(unsigned char)cRecved;
						data_index++;
					}
				}//���ж�
				//��ʼ����
				else
				{
					date_receive[data_index]=(uchar)cRecved;
					data_index++;
					if (data_index >= DATA_NUM)
					{
						data_index=0;
						baotou_flag=0;
					}
				};
				//�������ݸ���
				if (data_index >= DATA_NUM)
				{
					data_index=0;
					baotou_flag=0;
				}
				
				//std::cout << cRecved ; 
				continue;
			}
		
			if (data_index >= DATA_NUM)
				{
					data_index=0;
					baotou_flag=0;
				}

		}while(--BytesInQue);

		/*********************�������ݸ������ж�*******************/
		//�ٴγ�λ������
		if (data_index >= DATA_NUM)
				{
					data_index=0;
					baotou_flag=0;
				}
	}
}

#pragma comment(lib,"Winmm.lib")  
UINT timer_ID;
bool CSerialPort::OpenListenThread()
{
	timer_ID = timeSetEvent (1,1,timer_f,NULL,TIME_PERIODIC);//�жϲ���
	/** ����߳��Ƿ��Ѿ������� */ 
	//if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		/** �߳��Ѿ����� */ 
	//	return false;
	}

	//s_bExit = false;
	/** �߳�ID */ 
	//UINT threadId;
	/** �����������ݼ����߳� */ 
	/*m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, &threadId);
	if (!m_hListenThread)
	{
		return false;
	}*/
	/** �����̵߳����ȼ�,������ͨ�߳� */ 
	/*if (!SetThreadPriority(m_hListenThread, THREAD_PRIORITY_ABOVE_NORMAL))
	{
		return false;
	}*/
	
	//SetTimer(NULL,0,2/*һm��*/,TimerProc/*���ݻص�����ָ��*/);
	return true;

	  ;

}

bool CSerialPort::CloseListenTread()
{	
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
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

bool CSerialPort::ReadChar( char &cRecved )
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
	bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);
	if ((!bResult))
	{ 
		/** ��ȡ������,���Ը��ݸô�����������ԭ�� */ 
		DWORD dwError = GetLastError();

		/** ��մ��ڻ����� */ 
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** �뿪�ٽ��� */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return (BytesRead == 1);

}

bool CSerialPort::WriteData( unsigned char* pData, unsigned int length )
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
		DWORD dwError = GetLastError();
		/** ��մ��ڻ����� */ 
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** �뿪�ٽ��� */ 
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}


