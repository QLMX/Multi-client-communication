#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED
#include <iostream>
#include <winsock2.h>
#include <process.h>

#pragma comment(lib, "WS2_32.lib")
using namespace std;

//宏定义
#define	SERVERIP			"192.168.209.1"		//服务器IP
#define	SERVERPORT			6666			//服务器TCP端口
#define	MAX_NUM_BUF			60				//缓冲区的最大长度

//函数声明
BOOL InitClient(void);              //初始化
void InitMember(void);              //初始化全局变量
BOOL InitSockt(void);               //非阻塞套接字
BOOL ConnectServer(void);           //连接服务器
bool RecvLine(SOCKET s, char* buf); //读取一行数据
bool sendData(SOCKET s, char* str); //发送数据
void recvAndSend(void);             //数据处理函数
bool recvData(SOCKET s, char* buf);
void ShowConnectMsg(BOOL bSuc);     //连接打印函数
void ExitClient(void);              //退出服务器
DWORD __stdcall	RecvDataThread(void* pParam);
DWORD __stdcall	SendDataThread(void* pParam);
BOOL	CreateSendAndRecvThread(void);
void	InputAndOutput(void);


#endif
