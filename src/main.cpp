#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "TcpServer.h"
#include "Log.h"
#include <iostream>

int main(int argc, char* argv[])
{
#if 0
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    // 切换服务器的工作路径
    int ret = chdir(argv[2]);
    if (ret == -1) {
        perror("chdir");
    }
    unsigned short port = atoi(argv[1]);
    string directory = argv[2];
#else
    int ret = chdir(DIRECTORY);
    if (ret == -1) {
        perror("chdir");
    }
    unsigned short port = PORT;
    string directory = DIRECTORY;
#endif
    // 启动服务器
    TcpServer* server = new TcpServer(port, 16);
    server->setDirectory(DIRECTORY);
    server->run();    
    cout << "Server started..." << endl;
    return 0;
}
