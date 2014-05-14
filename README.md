chatserver
==========
using markdown syntax  
### usage:
\#启动Server程序，参数为端口号  
./chatserver \<port\>  
\#启动client程序，参数为Server所在机器的ip和Server监听的端口号  
./client \<ip\> \<port\>
    
### client支持的命令：
login yourname # 登陆  
look  # 查看当前用户  
logout # 退出  
其他输入均当做message发出去  

=====
**update 1.0.2**
服务端改成epoll模型，省去自己管理线程的麻烦，且为非阻塞，提高效率
主要改动为chatserver.cpp的run函数
**update 1.0.1**  
服务端发送消息时加入sep标记，sep为ASCII码的30  
客户端接受消息后根据sep切割，否则服务端连续发两条消息，客户端可能一次全部接受，这样无法区分消息边界
