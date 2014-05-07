#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
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

typedef void (ChatServer::*p_func)(char *arg, bool &is_logged, int connfd, ChatServer *p_session, std::string &user_name);

class ChatServer
{
    public:
        std::map<std::string, p_func> m_func;
        ChatServer(int p) : port(p), cur_thread_num(0)
    {
        for(int i = 0; i < MAX_THREAD_NUM; ++i)
        {
            connfd_arr[i] = -1;
        }
        m_func["login"] = &ChatServer::login;
        m_func["say"] = &ChatServer::say;
        m_func["logout"] = &ChatServer::logout;
        m_func["look"] = &ChatServer::look;
        m_func["help"] = &ChatServer::help;
    }
        void login(char *arg, bool &is_logged, int connfd, ChatServer *p_session, std::string &user_name);
        void say(char *arg, bool &is_looged, int connfd, ChatServer *p_session, std::string &user_name);
        void logout(char *arg, bool &is_looged, int connfd, ChatServer *p_session, std::string &user_name);
        void look(char *arg, bool &is_looged, int connfd, ChatServer *p_session, std::string &user_name);
        void help(char *arg, bool &is_looged, int connfd, ChatServer *p_session, std::string &user_name);
        void broadcase(char *msg, int msg_len);
        void analyse_cmd(char *buf, char *cmd, char *arg, bool is_logged);
        int run();
        int get_connfd(int connfd_index);
        void destroy_connfd(int connfd_index);
        int get_valid_connfd_index();
        int hasUser(const std::string &name);
        void removeUser(int connfd, const std::string &name);
        void addUser(int connfd, const std::string &name);
        int initSock();
        static void *talk_thread(void *);
        inline void decrease_thread()
        {
            --cur_thread_num;
            DEBUG("decrease_thread called, now thread_num is %d\n", cur_thread_num);
        }
        inline void increase_thread()
        {
            ++cur_thread_num;
            DEBUG("increase_thread called, now thread_num is %d\n", cur_thread_num);
        }
        inline int get_thread_num()
        {
            DEBUG("get_thread_num called, now thread_num is %d\n", cur_thread_num);
            return cur_thread_num;
        }

    private:
        std::map<int, std::string> m_users; //<connfd, user>
        std::set<std::string> s_users;
        pthread_t thread[MAX_THREAD_NUM];
        struct sockaddr_in servaddr;
        int listenfd;
        int connfd_arr[MAX_THREAD_NUM];
        int cur_thread_num;
        int port;
};

typedef struct _thread_para_t
{
    ChatServer *p_session;
    int connfd_index;
}thread_para_t;

#endif

