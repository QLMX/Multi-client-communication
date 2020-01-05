#include "server.h"
#include "sclient.h"


/**
 * ȫ�ֱ���
 */
char	dataBuf[MAX_NUM_BUF];				//д������
BOOL	bConning;							//��ͻ��˵�����״̬
BOOL    bSend;                              //���ͱ��λ
BOOL    clientConn;                         //���ӿͻ��˱��
SOCKET	sServer;							//�����������׽���
CRITICAL_SECTION  cs;			            //�������ݵ��ٽ�������
HANDLE	hAcceptThread;						//���ݴ����߳̾��
HANDLE	hCleanThread;						//���ݽ����߳�
ClIENTVECTOR clientvector;                  //�洢���׽���

/**
 * ��ʼ��
 */
BOOL initSever(void)
{
    //��ʼ��ȫ�ֱ���
	initMember();

	//��ʼ��SOCKET
	if (!initSocket())
		return FALSE;

	return TRUE;
}

/**
 * ��ʼ��ȫ�ֱ���
 */
void	initMember(void)
{
	InitializeCriticalSection(&cs);				            //��ʼ���ٽ���
	memset(dataBuf, 0, MAX_NUM_BUF);
	bSend = FALSE;
	clientConn = FALSE;
	bConning = FALSE;									    //������Ϊû������״̬
	hAcceptThread = NULL;									//����ΪNULL
	hCleanThread = NULL;
	sServer = INVALID_SOCKET;								//����Ϊ��Ч���׽���
	clientvector.clear();									//�������
}

/**
 *  ��ʼ��SOCKET
 */
bool initSocket(void)
{
	//����ֵ
	int reVal;

	//��ʼ��Windows Sockets DLL
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2,2),&wsData);

	//�����׽���
	sServer = socket(AF_INET, SOCK_STREAM, 0);
	if(INVALID_SOCKET== sServer)
		return FALSE;

	//�����׽��ַ�����ģʽ
	unsigned long ul = 1;
	reVal = ioctlsocket(sServer, FIONBIO, (unsigned long*)&ul);
	if (SOCKET_ERROR == reVal)
		return FALSE;

	//���׽���
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(SERVERPORT);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	reVal = bind(sServer, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if(SOCKET_ERROR == reVal )
		return FALSE;

	//����
	reVal = listen(sServer, CONN_NUM);
	if(SOCKET_ERROR == reVal)
		return FALSE;

	return TRUE;
}

/**
 *  ��������
 */
bool startService(void)
{
    BOOL reVal = TRUE;	//����ֵ

	showTipMsg(START_SERVER);	//��ʾ�û�����

	char cInput;		//�����ַ�
	do
	{
		cin >> cInput;
		if ('s' == cInput || 'S' == cInput)
		{
			if (createCleanAndAcceptThread())	//���ܿͻ���������߳�
			{
				showServerStartMsg(TRUE);		//�����̳߳ɹ���Ϣ
			}else{
				reVal = FALSE;
			}
			break;//����ѭ����
		}else{
			showTipMsg(START_SERVER);
		}

	} while(cInput != 's' && cInput != 'S'); //��������'s'����'S'�ַ�

    cin.sync();                     //������뻺����

	return reVal;

}

/**
 * ����������Դ�ͽ��ܿͻ��������߳�
 */
BOOL createCleanAndAcceptThread(void)
{
    bConning = TRUE;//���÷�����Ϊ����״̬

	//�����ͷ���Դ�߳�
	unsigned long ulThreadId;
	//�������տͻ��������߳�
	hAcceptThread = CreateThread(NULL, 0, acceptThread, NULL, 0, &ulThreadId);
	if( NULL == hAcceptThread)
	{
		bConning = FALSE;
		return FALSE;
	}
	else
    {
		CloseHandle(hAcceptThread);
	}
	//�������������߳�
	hCleanThread = CreateThread(NULL, 0, cleanThread, NULL, 0, &ulThreadId);
	if( NULL == hCleanThread)
	{
		return FALSE;
	}
	else
    {
		CloseHandle(hCleanThread);
	}
	return TRUE;
}

/**
 * ���ܿͻ�������
 */
DWORD __stdcall acceptThread(void* pParam)
{
    SOCKET  sAccept;							                        //���ܿͻ������ӵ��׽���
	sockaddr_in addrClient;						                        //�ͻ���SOCKET��ַ

	while(bConning)						                                //��������״̬
	{
		memset(&addrClient, 0, sizeof(sockaddr_in));					//��ʼ��
		int	lenClient = sizeof(sockaddr_in);				        	//��ַ����
		sAccept = accept(sServer, (sockaddr*)&addrClient, &lenClient);	//���ܿͻ�����
		if(INVALID_SOCKET == sAccept )
		{
			int nErrCode = WSAGetLastError();
			if(nErrCode == WSAEWOULDBLOCK)	                            //�޷��������һ�����赲���׽��ֲ���
			{
				Sleep(TIMEFOR_THREAD_SLEEP);
				continue;                                               //�����ȴ�
			}
			else
            {
				return 0;                                               //�߳��˳�
			}

		}
		else//���ܿͻ��˵�����
		{
		    clientConn = TRUE;          //�Ѿ������Ͽͻ���
		    CClient *pClient = new CClient(sAccept, addrClient);
		    EnterCriticalSection(&cs);
            //��ʾ�ͻ��˵�IP�Ͷ˿�
            char *pClientIP = inet_ntoa(addrClient.sin_addr);
            u_short  clientPort = ntohs(addrClient.sin_port);
            cout<<"Accept a client IP: "<<pClientIP<<"\tPort: "<<clientPort<<endl;
			clientvector.push_back(pClient);            //��������
            LeaveCriticalSection(&cs);

            pClient->StartRuning();
		}
	}
	return 0;//�߳��˳�
}

/**
 * ������Դ�߳�
 */
DWORD __stdcall cleanThread(void* pParam)
 {
    while (bConning)                  //��������������
	{
		EnterCriticalSection(&cs);//�����ٽ���

		//�����Ѿ��Ͽ������ӿͻ����ڴ�ռ�
		ClIENTVECTOR::iterator iter = clientvector.begin();
		for (iter; iter != clientvector.end();)
		{
			CClient *pClient = (CClient*)*iter;
			if (!pClient->IsConning())			//�ͻ����߳��Ѿ��˳�
			{
				iter = clientvector.erase(iter);	//ɾ���ڵ�
				delete pClient;				//�ͷ��ڴ�
				pClient = NULL;
			}else{
				iter++;						//ָ������
			}
		}
		if(clientvector.size() == 0)
        {
            clientConn = FALSE;
        }

		LeaveCriticalSection(&cs);          //�뿪�ٽ���

		Sleep(TIMEFOR_THREAD_HELP);
	}


	//������ֹͣ����
	if (!bConning)
	{
		//�Ͽ�ÿ������,�߳��˳�
		EnterCriticalSection(&cs);
		ClIENTVECTOR::iterator iter = clientvector.begin();
		for (iter; iter != clientvector.end();)
		{
			CClient *pClient = (CClient*)*iter;
			//����ͻ��˵����ӻ����ڣ���Ͽ����ӣ��߳��˳�
			if (pClient->IsConning())
			{
				pClient->DisConning();
			}
			++iter;
		}
		//�뿪�ٽ���
		LeaveCriticalSection(&cs);

		//�����ӿͻ����߳�ʱ�䣬ʹ���Զ��˳�
		Sleep(TIMEFOR_THREAD_HELP);
	}

	clientvector.clear();		//�������
	clientConn = FALSE;

	return 0;
 }

/**
 * ��������
 */
 void inputAndOutput(void)
 {
    char sendBuf[MAX_NUM_BUF];

    showTipMsg(INPUT_DATA);

    while(bConning)
    {
        memset(sendBuf, 0, MAX_NUM_BUF);		//��ս��ջ�����
        cin.getline(sendBuf,MAX_NUM_BUF);	//��������
        //��������
        handleData(sendBuf);
    }
 }


/**
 *  ѡ��ģʽ��������
 */
 void handleData(char* str)
 {
    CClient *sClient ;
    string recvsting;
    char cnum;                //��ʾ�������׽���
    int num;

    if(str != NULL)
    {
       if(!strncmp(WRITE, str, strlen(WRITE))) //�ж�����ָ���Ƿ�Ϊ
        {
            EnterCriticalSection(&cs);
            str += strlen(WRITE);
            cnum = *str++;
            num = cnum - '1';
            //������������
            if(num<clientvector.size())
            {
                sClient = clientvector.at(num);     //���͵�ָ���ͻ���
                strcpy(dataBuf, str);
                sClient->IsSend();
                LeaveCriticalSection(&cs);
            }
            else                                    //���ڷ�Χ
            {
                cout<<"The client isn't in scope!"<<endl;
                LeaveCriticalSection(&cs);
            }

        }
        else if(!strncmp(WRITE_ALL, str, strlen(WRITE_ALL)))
        {
            EnterCriticalSection(&cs);
            str += strlen(WRITE_ALL);
            strcpy(dataBuf, str);
            bSend = TRUE;
            LeaveCriticalSection(&cs);
        }
        else if('e'==str[0] || 'E'== str[0])     //�ж��Ƿ��˳�
        {
            bConning = FALSE;
            showServerExitMsg();
            Sleep(TIMEFOR_THREAD_EXIT);
            exitServer();
        }
        else
        {
            cout <<"Input error!!"<<endl;
        }

    }
 }

/**
 *  �ͷ���Դ
 */
void  exitServer(void)
{
	closesocket(sServer);					//�ر�SOCKET
	WSACleanup();							//ж��Windows Sockets DLL
}

void showTipMsg(int input)
{
    EnterCriticalSection(&cs);
	if (START_SERVER == input)          //����������
	{
		cout << "**********************" << endl;
		cout << "* s(S): Start server *" << endl;
		cout << "* e(E): Exit  server *" << endl;
		cout << "**********************" << endl;
		cout << "Please input:" ;

	}
	else if(INPUT_DATA == input)
    {
        cout << "*******************************************" << endl;
        cout << "* please connect clients,then send data   *" << endl;
		cout << "* write+num+data:Send data to client-num  *" << endl;
		cout << "*   all+data:Send data to all clients     *" << endl;
		cout << "*          e(E): Exit  server             *" << endl;
		cout << "*******************************************" << endl;
		cout << "Please input:" <<endl;
    }
	 LeaveCriticalSection(&cs);
}

/**
 * ��ʾ�����������ɹ���ʧ����Ϣ
 */
void  showServerStartMsg(BOOL bSuc)
{
	if (bSuc)
	{
		cout << "**********************" << endl;
		cout << "* Server succeeded!  *" << endl;
		cout << "**********************" << endl;
	}else{
		cout << "**********************" << endl;
		cout << "* Server failed   !  *" << endl;
		cout << "**********************" << endl;
	}

}

/**
 * ��ʾ�������˳���Ϣ
 */
void  showServerExitMsg(void)
{

	cout << "**********************" << endl;
	cout << "* Server exit...     *" << endl;
	cout << "**********************" << endl;
}

