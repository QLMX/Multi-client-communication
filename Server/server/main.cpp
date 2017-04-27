#include "server.h"

int main(int argc, char* argv[])
{
	//初始化服务器
	if (!initSever())
	{
		exitServer();
    	return SERVER_SETUP_FAIL;
	}

	//启动服务
	if (!startService())
	{
	    showServerStartMsg(FALSE);
		exitServer();
    	return SERVER_SETUP_FAIL;
	}

	 //处理数据
    inputAndOutput();

    //退出主线程，清理资源
	exitServer();

	return 0;
}
