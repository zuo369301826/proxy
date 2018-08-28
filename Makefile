
.PHONY:all

all:server tranfer

tranfer:epoll.cc tranfer.cc
	g++ $^ -o $@ -std=c++11 

server:epoll.cc socks5.cc
	g++ $^ -o $@ -std=c++11 

.PHONY:clear
	
	
clear:
	rm server tranfer

