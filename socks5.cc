#include "socks5.h"

// -----------------------------------------------------------------------
int Socks5Server::AuthHandle(int fd)//身份验证
{
  char buf[260];//发送版本（1）、标识符（1）、方法选择（1-255）
  int size = recv(fd, buf, 260, MSG_PEEK);
  if(size < 0)//recv 出错
    return -1;
  else if(size < 3) // 接受数据小于3  socks5 的数据最小为3  
    return 0;
  else
  {
    
    recv(fd, buf, size, 0);

    if(buf[0] != 0x05) // 版本号不正确
    {
      ErrorLog("not socks5");
      return -1;
    }
    return 1;
  } 
}

// -----------------------------------------------------------------------
int  Socks5Server::EstablishmentHandle(int fd)//建立连接
{
  char buf[256];// 版本号（1） CMD（1） RSV（1） ATYP(1)  服务端地址(变量)  端口号（2）  6 + 变量(最大63)
  int size = recv(fd, buf, 256, MSG_PEEK);
  if(size < 0)
  {
    return -1;
  }
  else if(size < 10)
  {
    return -2;
  }
  else 
  {
    char ip[4];
    char port[2];

    //先读出前4字节
    recv(fd, buf, 4, 0);
    char addr_type = buf[3];
    if(addr_type == 0x01) //ipv4
    {
      recv(fd, ip, 4, 0);//获取ip
      recv(fd, port, 2, 0);//获取port
    }
    else if(addr_type == 0x03)//domainname
    {
      char len = 0;
      recv(fd, &len, 1, 0); // 读取长度
      recv(fd, buf, len, 0); //读取域名
      buf[len] = '\0';
      struct hostent* hostptr = gethostbyname(buf);
      memcpy(ip, hostptr->h_addr, hostptr->h_length);
     
      recv(fd, port, 2, 0);//获取端口号

      TraceLog("domainname:%s",buf);  //打印要连接的域名
    }
    else if(addr_type == 0x04)//ipv6
    {
      ErrorLog("not support ipv6");
      return -1;
    }
    else
    {
      ErrorLog("invalid address type");
      return -1;
    }

    //开始连接服务器
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port =  *((uint64_t*)port) ; 
    memcpy(&addr.sin_addr, ip, 4);
 
    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverfd < 0)
    {
      ErrorLog("server socket");
      return -1;
    }
    
    if(connect(serverfd, (struct sockaddr*)&addr, sizeof addr) < 0)
    {
      ErrorLog("connect error");
      close(serverfd);
      return -1;
    } 
    return serverfd;
  }
}


// -----------------------------------------------------------------------
void  Socks5Server::ReadEpollEvent(int connectfd)// 读事件
{
  
  auto it = _fdConnectMap.find(connectfd);
  
  if(it != _fdConnectMap.end() )
  {
    Connect* con = it->second;
    if(con->_state == AUTH) // 身份验证 确定子连接
    {
      char reply[2];
      reply[0] = 0x05;
      int ret = AuthHandle(connectfd); // 进行身份验证
      if(ret == 0)
      {
        return ;
      }
      if(ret == 1)
      {
        reply[1] = 0x00;//表示无须密码
        con->_state = ESTABLISHMENT;
      }
      else if(ret == -1)
      {
        reply[1] = 0xFF;
        RemoveConnect(connectfd);//删除文件描述符和管道
      }

      if(send(connectfd, reply, 2, 0) != 2)
      {
        ErrorLog("auth reply");
      }
    }



    else if(con->_state == ESTABLISHMENT) // 确定连接对端
    {
      char reply[10] = {0};
      reply[0] = 0x05;

      int serverfd = EstablishmentHandle(connectfd);
      if( serverfd == -1)
      {
        reply[1] = 0x01;
        RemoveConnect(connectfd);//删除文件描述符和管道
      }
      else if(serverfd == -2)
      {
        return ;
      }
      else 
      {
        reply[1] = 0x00;//正确
        reply[3] = 0x01;//表示 ipv4 
      }

      if(send(connectfd, reply, 10, 0) != 10)
      {
        ErrorLog("establishment reply");
      }
      
      if(serverfd >= 0 )
      {
        SetNonblockint(serverfd);
        Epoll_Op(serverfd,EPOLLIN ,EPOLL_CTL_ADD);  
        con->_serverChannel.fd = serverfd; 
        con->_ref++;
        con->_state = FORWARDING;
        _fdConnectMap[serverfd] = con;
      }
    }
    
    else if(con->_state == FORWARDING) // 消息传输
    {
      Channel* clientChannel = &con->_clientChannel;
      Channel* serverChannel = &con->_serverChannel;

      if(connectfd == serverChannel->fd)
      {
        swap(clientChannel, serverChannel);
      }

      Forwarding(clientChannel->fd, serverChannel->fd); 
    }
     
    else 
    {
      assert(false);
    }
  }
  else 
  {
    assert(false);
  }
}


// -----------------------------------------------------------------------
void Socks5Server::ConnectEpollEvent(int connectfd)// 连接事件
{
  TraceLog("new conenct event:%d", connectfd);

  SetNonblockint(connectfd);

  Epoll_Op(connectfd, EPOLLIN, EPOLL_CTL_ADD);

  Connect* con = new Connect;

  con->_clientChannel.fd = connectfd;

  con->_ref++;

  _fdConnectMap[connectfd] = con; 

}


int main()
{
  Socks5Server server(8001);
  server.Start();
  return 0;
}

