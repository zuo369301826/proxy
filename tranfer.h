#pragma once

#include "common.h"
#include "epoll.h"

class TranferServer:public EpollServer
{
public:
  TranferServer(int selfport, const char* sock5ip, int socks5port)
    :EpollServer(selfport)
  {
    memset(&_socks5addr, 0, sizeof _socks5addr );
    _socks5addr.sin_family = AF_INET;
    _socks5addr.sin_port = htons(socks5port);
    _socks5addr.sin_addr.s_addr = inet_addr(sock5ip); 
  }

  void ConnectEpollEvent(int connectfd);
  void ReadEpollEvent(int connectfd); 

protected: 
    struct sockaddr_in _socks5addr;
};
