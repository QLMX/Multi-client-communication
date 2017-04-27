#include <process.h>
#include <stdio.h>
#include "sclient.h"
#include "server.h"

//extern BOOL bSend;
//extern char	dataBuf[MAX_NUM_BUF];
/*
 * 构造函数
 */
CClient::CClient(const SOCKET sClient, const sockaddr_in &addrClient)
{
	//初始化变量
	m_hThreadRecv = NULL;
	m_hThreadSend = NULL;
	m_socket = sClient;
	m_addr = addrClient;
	m_bConning = FALSE;
	m_bExit = FALSE;
	m_bSend = FALSE;
	memset(m_data.buf, 0, MAX_NUM_BUF);

	//创建事件
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);//手动设置信号状态，初始化为无信号状态

	//初始化临界区
	InitializeCriticalSection(&m_cs);
}
/*
 * 析构函数
 */
CClient::~CClient()
{
	closesocket(m_socket);			//关闭套接字
	m_socket = INVALID_SOCKET;		//套接字无效
	DeleteCriticalSection(&m_cs);	//释放临界区对象
	CloseHandle(m_hEvent);			//释放事件对象
}

/*
 * 创建发送和接收数据线程
 */
BOOL CClient::StartRuning(void)
{
	m_bConning = TRUE;//设置连接状态

	//创建接收数据线程
	unsigned long ulThreadId;
	m_hThreadRecv = CreateThread(NULL, 0, RecvDataThread, this, 0, &ulThreadId);
	if(NULL == m_hThreadRecv)
	{
		return FALSE;
	}else{
		CloseHandle(m_hThreadRecv);
	}

	//创建接收客户端数据的线程
	m_hThreadSend =  CreateThread(NULL, 0, SendDataThread, this, 0, &ulThreadId);
	if(NULL == m_hThreadSend)
	{
		return FALSE;
	}else{
		CloseHandle(m_hThreadSend);
	}

	return TRUE;
}


/*
 * 接收客户端数据
 */
DWORD  CClient::RecvDataThread(void* pParam)
{
	CClient *pClient = (CClient*)pParam;	//客户端对象指针
	int		reVal;							//返回值
	char	temp[MAX_NUM_BUF];				//临时变量


	while(pClient->m_bConning)				//连接状态
	{
	    memset(temp, 0, MAX_NUM_BUF);
		reVal = recv(pClient->m_socket, temp, MAX_NUM_BUF, 0);	//接收数据

		//处理错误返回值
		if (SOCKET_ERROR == reVal)
		{
			int nErrCode = WSAGetLastError();

			if ( WSAEWOULDBLOCK == nErrCode )	//接受数据缓冲区不可用
			{
				continue;						//继续循环
			}else if (WSAENETDOWN == nErrCode ||//客户端关闭了连接
					 WSAETIMEDOUT == nErrCode ||
					WSAECONNRESET == nErrCode )
			{
				break;							//线程退出
			}
		}

		//客户端关闭了连接
		if ( reVal == 0)
		{
			break;
		}

		//收到数据
		if (reVal > 0)
		{
		    EnterCriticalSection(&pClient->m_cs);
		    char *pClientIP = inet_ntoa(pClient->m_addr.sin_addr);
            u_short  clientPort = ntohs(pClient->m_addr.sin_port);
			cout<<"IP: "<<pClientIP<<"\tPort: "<<clientPort<<":"<<temp<<endl;      //输出显示数据
            LeaveCriticalSection(&pClient->m_cs);

			memset(temp, 0, MAX_NUM_BUF);	//清空临时变量
		}

	}
	pClient->m_bConning = FALSE;			//与客户端的连接断开
	return 0;								//线程退出
}

/*
 * @des: 向客户端发送数据
 */
DWORD CClient::SendDataThread(void* pParam)
{
	CClient *pClient = (CClient*)pParam;//转换数据类型为CClient指针
	while(pClient->m_bConning)//连接状态
	{
        if(pClient->m_bSend || bSend)
        {
			//进入临界区
			EnterCriticalSection(&pClient->m_cs);
			//发送数据
			int val = send(pClient->m_socket, dataBuf, strlen(dataBuf),0);
			//处理返回错误
			if (SOCKET_ERROR == val)
			{
				int nErrCode = WSAGetLastError();
				if (nErrCode == WSAEWOULDBLOCK)//发送数据缓冲区不可用
				{
					continue;
				}else if ( WSAENETDOWN == nErrCode ||
						  WSAETIMEDOUT == nErrCode ||
						  WSAECONNRESET == nErrCode)//客户端关闭了连接
				{
					//离开临界区
					LeaveCriticalSection(&pClient->m_cs);

					pClient->m_bConning = FALSE;	//连接断开
//					pClient->m_bExit = TRUE;		//线程退出
					pClient->m_bSend = FALSE;
					break;
				}else {
					//离开临界区
					LeaveCriticalSection(&pClient->m_cs);
					pClient->m_bConning = FALSE;	//连接断开
//					pClient->m_bExit = TRUE;		//线程退出
					pClient->m_bSend = FALSE;
					break;
				}
			}
			//成功发送数据
			//离开临界区
			memset(dataBuf, 0, MAX_NUM_BUF);		//清空接收缓冲区
			LeaveCriticalSection(&pClient->m_cs);
			//设置事件为无信号状态
			pClient->m_bSend = FALSE;
			bSend = FALSE;
		}

	}

	return 0;
}

