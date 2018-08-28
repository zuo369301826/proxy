#include"epoll.h"

void EpollServer::Start()
{
  _listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if(_listenfd < 0)
  {
    ErrorLog("create socket");
    return ;
  }

  //设置端口重用
  int opt = 1;
  setsockopt(_listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
  
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(_port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(_listenfd, (struct sockaddr*)&addr, sizeof addr) < 0)
  {
    ErrorLog("bind socket");
    return ;
  }

  if(listen(_listenfd, 100000) < 0)
  {
    ErrorLog("listen");
    return;
  }

  TraceLog("epoll server listen on %d", _port);

  _eventfd = epoll_create(100000);
  if(_eventfd < 0)
  {
    ErrorLog("epoll_create");
    return ;
  }

  //设置监听描述符
  SetNonblockint(_listenfd);

  //添加读事件
  Epoll_Op(_listenfd, EPOLLIN, EPOLL_CTL_ADD);

  //启动epoll
  EventLoop();
}

//启动 epoll  
void EpollServer::EventLoop()
{
    struct epoll_event events[100000];
    while(1)
    {
      int n = epoll_wait(_eventfd, events, 100000, 0);
      for(int i=0; i < n; ++i)
      {
        if( events[i].data.fd == _listenfd)
        {
          struct sockaddr_in addr;
          socklen_t len = sizeof addr;
          int connectfd = accept(_listenfd,(struct sockaddr*)&addr, &len);
          ConnectEpollEvent(connectfd);
        }
        else if(events[i].events & EPOLLIN)
        {
          ReadEpollEvent(events[i].data.fd);
        }
        else if(events[i].events & EPOLLOUT)
        {
          WriteEpollEvent(events[i].data.fd);
        }
        else
        {
          ErrorLog("event: %d", events[i].data.fd);
        } 
      }
    }
}

void EpollServer::RemoveConnect(int fd)
{
  //close(fd);
  Epoll_Op(fd, 0, EPOLL_CTL_DEL);
  auto it = _fdConnectMap.find(fd);
  if(it != _fdConnectMap.end())
  {
    auto* con = it->second;
    if(--con->_ref == 0)
    {
      delete con;
      _fdConnectMap.erase(it);
    }
  }
  else 
  {
    assert(false);
  }

}
void EpollServer::Forwarding(int clientfd, int serverfd) //转发数据
{
  char buf[4096] = {0};
  int rlen = recv(clientfd, buf, 4096, 0);

  if(rlen <  0)
  {
    ErrorLog("recv : %d", clientfd);
  }
  else if(rlen == 0)
  {
    shutdown(serverfd, SHUT_WR);
    RemoveConnect(clientfd);
  }
  else 
  {
    buf[rlen] = '\0';
    Send_Loop(serverfd, buf, rlen);
  }
}

//循环发送
void EpollServer::Send_Loop(int fd, const  char* buf, int len)
{
  int slen = send(fd, buf, len, 0);
  if(slen < 0)
  {
    ErrorLog("send to %d", fd);
  }
  else if(slen < len)
  {
    TraceLog("recv %d bytes, send %d bytes, left %d send in loop", len, slen, len-slen);
    auto it = _fdConnectMap.find(fd);
    if(it != _fdConnectMap.end())
    {
      Connect* con = it->second;
      Channel* channel = &con->_clientChannel;
      if(fd == con->_serverChannel.fd)
      {
        channel = &con->_serverChannel;
      }

      int events = EPOLLOUT | EPOLLIN || EPOLLONESHOT;
      Epoll_Op(fd, events, EPOLL_CTL_MOD);
      channel->buff.append(buf+slen);
    }
    else
    {
      assert(false);
    }
  }

}

void EpollServer::WriteEpollEvent(int fd)
{
  auto it = _fdConnectMap.find(fd);
  if(it != _fdConnectMap.end())
  {
    Connect* con = it->second;
    Channel* channel = &con->_clientChannel;
    if(fd == con->_serverChannel.fd)
    {
      channel = &con->_serverChannel;
    }

    string buff;
    buff.swap(channel->buff);
    Send_Loop(fd, buff.c_str(), buff.size());
  }
}

