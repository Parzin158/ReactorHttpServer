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
    // �л��������Ĺ���·��
    chdir(argv[2]);
#else
    unsigned short port = PORT;
    chdir(DIRECTORY);
#endif
    // ����������
    TcpServer* server = new TcpServer(port, 5);
    server->setDirectory(DIRECTORY);
    server->run();    
    cout << "Server started..." << endl;

    return 0;
}