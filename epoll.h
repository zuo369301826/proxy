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

    //操作epoll
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

    //设置文件描述符为非阻塞
    void SetNonblockint(int fd)
    {
      int flags = fcntl(fd, F_GETFL, 0);//设置监听
      if(flags == -1)
        ErrorLog("SetNonblockint: F_GETFL");

      flags |= O_NONBLOCK;
      int s = fcntl(fd, F_SETFL, flags);
      if(s < 0)
        ErrorLog("SetNonblockint: F_SETEL");
    }
 
    struct Channel
    {
      int fd;
      Channel()
        :fd(-1)
      {}
    };
    
    //状态 
    enum Sock5State
    {
      AUTH,
      ESTABLISHMENT,
      FORWARDING
    };

    struct Connect //管道
    {
      Sock5State _state;//状态
      Channel _clientChannel; //客户端文件描述符
      Channel _serverChannel; //服务端文件描述符
      int _ref; //连接数

      Connect()
        :_state(AUTH),
        _ref(0)
        {}
    };

    virtual void WriteEpollEvent(int connectfd);// 写事件
    virtual void ReadEpollEvent(int connectfd) = 0;// 读事件
    virtual void ConnectEpollEvent(int connectfd) = 0;// 连接事件
    
    void Start();//服务器启动函数
    void EventLoop();//设置epoll

  private:
    int _port; //端口号
    int _listenfd; //监听套接字 
    int _eventfd; //事件描述符

    static const size_t _MAX_EVENTS; //最大文件描述符
};
