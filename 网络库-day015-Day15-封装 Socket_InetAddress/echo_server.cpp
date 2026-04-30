


#include "Socket.h"
#include <iostream>
#include <cstring>
#include <unistd.h>


int main()
{
    // 1.创建socket
    Socket listen;

    // 2.设置地址重用
    listen.SetReuseAddr();

    // 3.绑定地址
    InetAddress addr(8888);
    if(false == listen.Bind(addr))
    {
         std::cerr << "listen_Socket bind error \n";
         return 1;
    }


    // 4.监听
    if(false == listen.Listen())
    {
        std::cerr << "listen_Socket listen error \n";
        return 1;
    }

    

    // 5.接受连接并echo
    while(true)
    {
        ssize_t clientFd = listen.Accept();
        if(clientFd < 0)
        {
            std::cerr << "accept error \n";
            continue;
        }

        Socket clientSocket(clientFd);


        // 6.0 接受并回发
        char buffer[4096] = {0};
        while(true)
        {
            ssize_t n = clientSocket.Read(buffer, sizeof(buffer));
            if(n <= 0)
            {
                break;
            }

           clientSocket.Write(buffer, n);
        }
    }

   listen.Close();
    return 0;
}