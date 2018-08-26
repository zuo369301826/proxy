#pragma once

#include "epoll.h"

class Socks5Server: public EpollServer
{
  public:
    Sock5Server(int port)
      :EpollServer(port)
    {}

    int AuthHandle(int fd);//身份验证
    int EstablishmentHandle(int fd);

    void ReadEpollEvent(int connectfd);// 读事件
    void ConnectEpollEvent(int connectfd);// 连接事件
};

