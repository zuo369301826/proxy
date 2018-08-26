#pragma once
#include "common.h"

class EpollServer
{
  public:
    EpollServer(int port)
      :_port(port),
       _listenfd(-1),
       _eventfd(-1)
    {}

    ~EpollServer()
    {
      close(_listenfd);
    }

    void Epoll_Op(int fd, int events, int op)
    {
      struct epoll_event event;
      event.events = events;
      event.data.fd = fd;
      if( epoll_ctl(_eventfd, op, fd, &event) < 0)
      {
        ErrorLog("epoll_ctl()",op, fd);
      }
    }

    void SetNonblockint(int fd)
    {
      int flags = fcntl(fd, F_GETFL, 0);//设置监听
      

    }
  
    virtual void WriteEpollEvent();// 写事件
    virtual void ReadEpollEvent() = 0;// 读事件
    virtual void ConnectEpollEvent() = 0;// 连接事件
    
    void Start();//服务器启动函数

  private:
    int _port; //端口号
    int _listenfd; //监听套接字 
    int _eventfd; //事件描述符

    static const size_t _MAX_EVENTS; //最大文件描述符
};
