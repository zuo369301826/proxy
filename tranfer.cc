#include "tranfer.h"


void TranferServer::ConnectEpollEvent(int connectfd)
{
  int serverfd = socket(AF_INET, SOCK_STREAM, 0);
  if(serverfd == -1)
  {
    ErrorLog("socket");
    return ;
  }
  if(connect(serverfd, (struct sockaddr*)&_socks5addr, sizeof _socks5addr) < 0)
  {
    ErrorLog("connect socks5");
    return;
  }
 
  SetNonblockint(serverfd);
  Epoll_Op(serverfd, EPOLLIN, EPOLL_CTL_ADD);

  SetNonblockint(connectfd);
  Epoll_Op(connectfd, EPOLLIN, EPOLL_CTL_ADD);

  Connect* con = new Connect;
  con->_state = FORWARDING;

  con->_serverChannel.fd = serverfd;
  con->_ref++;
  _fdConnectMap[serverfd] = con;
  
  con->_clientChannel.fd = connectfd;
  con->_ref++;
  _fdConnectMap[connectfd] = con;
}

void TranferServer::ReadEpollEvent(int connectfd)
{
  auto it = _fdConnectMap.find(connectfd);
  if(it != _fdConnectMap.end())
  {
    Connect* con = it->second;
    Channel* clientChannel = &con->_clientChannel;
    Channel* serverChannel = &con->_serverChannel;

    bool sendencry = true, recvdecrypt = false;
    if(connectfd == con->_serverChannel.fd)
    {
      swap(clientChannel, serverChannel);
      swap(sendencry, recvdecrypt);
    }

    Forwarding(clientChannel->fd, serverChannel->fd);

  }
  else 
  {
    assert(false);
  }
}

int main()
{
  TranferServer server(8000, "127.0.0.1", 8001);
  server.Start();
}
