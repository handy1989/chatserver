#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_THREAD_NUM 100
#define MAX_LINE_LEN 4096

#define __DEBUG__ 1
#define DEBUG(argv, format...)do{\
    if (__DEBUG__){\
        fprintf(stderr, argv, ##format);\
    }\
}while(0)

class ChatServer;

typedef void (ChatServer::*p_func)(char *arg, bool is_logged, int connfd, ChatServer *p_session, std::string &user_name);

class ChatServer
{
    public:
        std::map<std::string, p_func> m_func;
        ChatServer(int p) : port(p), cur_user_num(0), cur_connect_num(0)
    {
        m_func["login"] = &ChatServer::login;
        m_func["say"] = &ChatServer::say;
        m_func["logout"] = &ChatServer::logout;
        m_func["look"] = &ChatServer::look;
        m_func["help"] = &ChatServer::help;
    }
        void login(char *arg, bool is_logged, int connfd, ChatServer *p_session, std::string &user_name);
        void say(char *arg, bool is_looged, int connfd, ChatServer *p_session, std::string &user_name);
        void logout(char *arg, bool is_looged, int connfd, ChatServer *p_session, std::string &user_name);
        void look(char *arg, bool is_looged, int connfd, ChatServer *p_session, std::string &user_name);
        void help(char *arg, bool is_looged, int connfd, ChatServer *p_session, std::string &user_name);
        void broadcase(char *msg, int msg_len);
        void analyse_cmd(char *buf, char *cmd, char *arg, bool is_logged);
        int run();
        int eventAccept();
        int eventRecv(char *line, struct epoll_event &event);
        int eventSend(char *line, struct epoll_event &event);
        int hasUser(const std::string &name);
        void removeUser(int connfd,  bool is_logged);
        void addUser(int connfd, const std::string &name);
        int initSock();
        inline bool isLogged(int connfd)
        {
            std::map<int, bool>::iterator it = m_islogged.find(connfd);
            return it == m_islogged.end() ? false : m_islogged[connfd];
        }
        inline void setLogged(int connfd, bool is_logged)
        {
            m_islogged[connfd] = is_logged;
        }
        inline void addConnfd(int connfd)
        {
            s_connfd.insert(connfd);
            m_islogged[connfd] = false;
            ++cur_connect_num;
            DEBUG("new connect, connect num[%d] user num[%d]\n", cur_connect_num, cur_user_num);
        }
        inline void removeConnfd(int connfd)
        {
            s_connfd.erase(connfd);
            m_islogged.erase(connfd);
            --cur_connect_num;
            DEBUG("new connect, connect num[%d] user num[%d]\n", cur_connect_num, cur_user_num);
        }
        inline int get_user_num()
        {
            return cur_user_num;
        }
        inline int get_connect_num()
        {
            return cur_connect_num;
        }

    private:
        std::map<int, std::string> m_users; //<connfd, user>
        std::set<std::string> s_users;
        std::map<int, bool> m_islogged; //<connfd, is_logged>
        //pthread_t thread[MAX_THREAD_NUM];
        struct sockaddr_in servaddr, clientaddr;
        int listenfd;
        std::set<int> s_connfd;
        int port;
        int cur_connect_num;
        int cur_user_num;
        int epfd;
        socklen_t client;
        struct epoll_event ev, events[20];
};

typedef struct _thread_para_t
{
    ChatServer *p_session;
    int connfd_index;
}thread_para_t;

int setnoblock(int);
#endif

