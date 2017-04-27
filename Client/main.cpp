#include <iostream>
#include "client.h"

using namespace std;

int main()
{
    	//初始化
	if (!InitClient())
	{
		ExitClient();
	}

	//连接服务器
	if(ConnectServer())
	{
		ShowConnectMsg(TRUE);
	}
	else
    {
		ShowConnectMsg(FALSE);
		ExitClient();
	}

	//创建发送和接收数据线程
	if (!CreateSendAndRecvThread())
	{
		ExitClient();

	}

	//用户输入数据和显示结果
	InputAndOutput();
//
//	//退出
//	ExitClient();
//
	return 0;
}
