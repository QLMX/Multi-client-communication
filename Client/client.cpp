#include "client.h"

//变量
SOCKET	sClient;							//套接字
HANDLE	hThreadSend;						//发送数据线程
HANDLE	hThreadRecv;						//接收数据线程
char    bufSend[MAX_NUM_BUF];				//发送数据缓冲区
BOOL    bSend = FALSE;                      //发送标记位
BOOL	bConnecting;						//与服务器的连接状态
HANDLE	arrThread[2];						//子线程数组
CRITICAL_SECTION cs;					//临界区对象，锁定bufSend

/**
 *	初始化
 */
BOOL InitClient(void)
{
	//初始化全局变量
	InitMember();

	//创建SOCKET
	if (!InitSockt())
	{
		return FALSE;
	}

	return TRUE;
}
/**
 * 初始化全局变量
 */
void InitMember(void)
{
	InitializeCriticalSection(&cs);

	sClient = INVALID_SOCKET;	//套接字
    hThreadRecv = NULL;			//接收数据线程句柄
	hThreadSend = NULL;			//发送数据线程句柄
	bConnecting = FALSE;		//为连接状态

    //初始化数据缓冲区
	memset(bufSend, 0, MAX_NUM_BUF);
	memset(arrThread, 0, 2);
}

/**
 * 创建非阻塞套接字
 */
BOOL  InitSockt(void)
{
	int			reVal;	//返回值
	WSADATA		wsData;	//WSADATA变量
	reVal = WSAStartup(MAKEWORD(2,2),&wsData);//初始化Windows Sockets Dll

	//创建套接字
	sClient = socket(AF_INET, SOCK_STREAM, 0);
	if(INVALID_SOCKET == sClient)
		return FALSE;


	//设置套接字非阻塞模式
	unsigned long ul = 1;
	reVal = ioctlsocket(sClient, FIONBIO, (unsigned long*)&ul);
	if (reVal == SOCKET_ERROR)
		return FALSE;

	return TRUE;
}

/**
 * 连接服务器
 */
BOOL ConnectServer(void)
{
	int reVal;			//返回值
	sockaddr_in serAddr;//服务器地址
	//输入要连接的主机地址
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(SERVERPORT);
    serAddr.sin_addr.S_un.S_addr = inet_addr(SERVERIP);

	while(true)
	{
		//连接服务器
		reVal = connect(sClient, (struct sockaddr*)&serAddr, sizeof(serAddr));
		//处理连接错误
		if(SOCKET_ERROR == reVal)
		{
			int nErrCode = WSAGetLastError();
			if( WSAEWOULDBLOCK == nErrCode || WSAEINVAL == nErrCode)    //连接还没有完成
			{
				continue;
			}
			else if (WSAEISCONN == nErrCode)//连接已经完成
			{
				break;
			}
			else//其它原因，连接失败
			{
				return FALSE;
			}
		}

		if ( reVal == 0 )//连接成功
			break;
	}

	bConnecting = TRUE;

	return TRUE;
}
/**
 * 创建发送和接收数据线程
 */
BOOL	CreateSendAndRecvThread(void)
{
	//创建接收数据的线程
	unsigned long ulThreadId;
	hThreadRecv = CreateThread(NULL, 0, RecvDataThread, NULL, 0, &ulThreadId);
	if (NULL == hThreadRecv)
		return FALSE;

	//创建发送数据的线程
	hThreadSend = CreateThread(NULL, 0, SendDataThread, NULL, 0, &ulThreadId);
	if (NULL == hThreadSend)
		return FALSE;

	//添加到线程数组
	arrThread[0] = hThreadRecv;
	arrThread[1] = hThreadSend;
	return TRUE;
}
/**
 * 接收数据线程
 */
DWORD __stdcall	RecvDataThread(void* pParam)
{
	int		reVal;				    //返回值
	char    bufRecv[MAX_NUM_BUF];   //接收数据缓冲区

	while(bConnecting)			    //连接状态
	{
        memset(bufRecv, 0, MAX_NUM_BUF);
		reVal = recv(sClient, bufRecv, MAX_NUM_BUF, 0);//接收数据
		if (SOCKET_ERROR == reVal)
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK == nErrCode)			//接受数据缓冲区不可用
			{
				continue;							//继续接收数据
			}else{
				bConnecting = FALSE;
				return 0;							//线程退出
			}
		}

		if ( reVal == 0)							//服务器关闭了连接
		{
			ShowConnectMsg(FALSE);
            bConnecting = FALSE;
            bSend = FALSE;
            memset(bufRecv, 0, MAX_NUM_BUF);		//清空接收缓冲区
            ExitClient();
			return 0;								//线程退出
		}
		if(reVal > 0)
        {

            if(('E'==bufRecv[0] || 'e'==bufRecv[0]))     //判断是否退出
            {
                ShowConnectMsg(FALSE);
                bConnecting = FALSE;
                bSend = FALSE;
                memset(bufRecv, 0, MAX_NUM_BUF);		//清空接收缓冲区
                ExitClient();
            }
            //显示数据
            cout<<bufRecv<<endl;
        }
	}
	return 0;
}
/**
 * 发送数据线程
 */
DWORD __stdcall	SendDataThread(void* pParam)
{
	while(bConnecting)						//连接状态
	{
		if (bSend)						//发送数据
		{
            EnterCriticalSection(&cs);	//进入临界区
			while(TRUE)
            {
                int val = send(sClient, bufSend, MAX_NUM_BUF,0);

                //处理返回错误
                if (SOCKET_ERROR == val)
                {
                    int nErrCode = WSAGetLastError();
                    if(WSAEWOULDBLOCK == nErrCode)		//发送缓冲区不可用
                    {
                        continue;						//继续循环
                    }else
                    {
                        LeaveCriticalSection(&cs);	//离开临界区
                        return 0;
                    }
                }

                    bSend = FALSE;			//发送状态
                    break;					//跳出for
            }
            LeaveCriticalSection(&cs);	//离开临界区
		}
    }
	return 0;
}
/**
 * 输入数据和显示结果
 */
void	InputAndOutput(void)
{
    char cInput[MAX_NUM_BUF];	//用户输入缓冲区
    while(bConnecting)			//连接状态
	{
		memset(cInput, 0, MAX_NUM_BUF);
		cin >> cInput;			        //输入表达式
        EnterCriticalSection(&cs);		//进入临界区
		memcpy(bufSend, cInput, strlen(cInput));
		LeaveCriticalSection(&cs);		//离开临界区
		bSend = TRUE;
	}
}

/**
 * 客户端退出
 */
void ExitClient(void)
{
	DeleteCriticalSection(&cs);
    CloseHandle(hThreadRecv);
	CloseHandle(hThreadSend);
    memset(bufSend, 0, MAX_NUM_BUF);
	closesocket(sClient);
	WSACleanup();
}

/**
 * 显示连接服务器失败信息
 */
void ShowConnectMsg(BOOL bSuc)
{
	if (bSuc)
	{
		cout << "* Succeed to connect server! *" << endl;
	}
	else
    {
		cout << "* Client has to exit! *" << endl;
	}
}
