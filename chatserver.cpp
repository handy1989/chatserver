#include "strtools.h"
#include "chatserver.h"

using std::vector;
using std::string;
using std::map;

int setnoblocking(int sock)
{
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(sock, GETFL)");
        return 1;
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        perror("fcntl(sock, SETFL, opts)");
        return 1;
    }
}


int ChatServer::hasUser(const string &name)
{
    return s_users.count(name); 
}

void ChatServer::addUser(int connfd, const string &name)
{
    m_users[connfd] = name;
    s_users.insert(name);
}

void ChatServer::removeUser(int connfd, const string &name, bool is_logged)
{
    if (is_logged)
    {
        m_users.erase(connfd);
        s_users.erase(name);
    }
}

void ChatServer::destroy_connfd(int connfd_index)
{
}

int ChatServer::get_connfd(int connfd_index)
{
    return 0;
}


void ChatServer::login(char *arg, bool &is_logged, int connfd, ChatServer *p_session, std::string &user_name)
{
    char ret_buf[MAX_LINE_LEN];
    //snprintf(ret_buf, MAX_LINE_LEN, "your're loggin%c%c", 30, 0);
    //send(connfd, ret_buf, strlen(ret_buf), 0);
    DEBUG("log name[%s]", arg);
    if (is_logged)
    {
        say(arg, is_logged, connfd, p_session, user_name);
        return ;
    }
    if (hasUser(arg))
    {
        snprintf(ret_buf, MAX_LINE_LEN, "User[%s] already exits. Please try again.%c%c", arg, 30, 0);
        send(connfd, ret_buf, strlen(ret_buf), 0);
    }
    else
    {
        addUser(connfd, arg);
        user_name = arg;
        snprintf(ret_buf, MAX_LINE_LEN, "%s joins the room!%c%c", arg, 30, 0);
        broadcase(ret_buf, strlen(ret_buf));
        is_logged = true;
    }
    DEBUG("ret_buf[%s]", ret_buf);

}

void ChatServer::broadcase(char *msg, int msg_len)
{
    int ret = 0;
    if (NULL == msg)
    {
        return ;
    }
    map<int ,string>::iterator it;
    for(it = m_users.begin(); it != m_users.end(); ++it)
    {
        ret = send(it->first, msg, msg_len, 0);
        if (ret < 0)
        {
            fprintf(stderr, "send message to connfd[%d] failed!\n", it->first);
        }
    }
}

void ChatServer::look(char *arg, bool &is_logged, int connfd, ChatServer *p_session, std::string &user_name)
{
    DEBUG("look\n");
    char ret_buf[MAX_LINE_LEN];
    if (!is_logged)
    {    
        snprintf(ret_buf, MAX_LINE_LEN, "You haven't been logged in. Please login using command \"login <name>\" or type \"help\" for help.%c%c", 30, 0);
        int len = strlen(ret_buf);
        DEBUG("ret_buf[%s] len[%d]\n", ret_buf, len);
        send(connfd, ret_buf, strlen(ret_buf), 0);
    }
    else
    {
        string str_ret_buf("user list:");
        char sep[10];
        sprintf(sep, "%c", 30); //分隔符ascii=30
        std::set<std::string>::iterator it;
        for (it = s_users.begin(); it != s_users.end(); ++it)
        {
            str_ret_buf = str_ret_buf + sep + *it;
        }
        send(connfd, str_ret_buf.c_str(), str_ret_buf.size(), 0);
    }
}

void ChatServer::say(char *arg, bool &is_logged, int connfd, ChatServer *p_session, std::string &user_name)
{
    //send(connfd, "you're saying.", 100, 0);
    DEBUG("saying. log_flag[%d]\n", is_logged);
    char ret_buf[MAX_LINE_LEN];
    int msg_len = 0;
    int ret = 0;
    if (!is_logged)
    {
        snprintf(ret_buf, MAX_LINE_LEN, "You haven't been logged in. Please login using command \"login <name>\" or type \"help\" for help.%c%c", 30, 0);
        int len = strlen(ret_buf);
        DEBUG("ret_buf[%s] len[%d]\n", ret_buf, len);
        send(connfd, ret_buf, strlen(ret_buf), 0);
    }
    else
    {
        std::string user_name = m_users[connfd];
        snprintf(ret_buf, MAX_LINE_LEN, "[%s]%s%c%c", user_name.c_str(), arg, 30, 0);
        DEBUG("ret_buf[%s]\n", ret_buf);
        p_session->broadcase(ret_buf, strlen(ret_buf));
    }
}

void ChatServer::logout(char *arg, bool &is_logged, int connfd, ChatServer *p_session, std::string &user_name)
{
    //send(connfd, "you're logout", 100, 0);
    if (!is_logged)
    {
        say(arg, is_logged, connfd, p_session, user_name);
        return ;
    }
    else
    {
        char ret_buf[MAX_LINE_LEN];
        std::string user = m_users[connfd];
        removeUser(connfd, user, is_logged);
        snprintf(ret_buf, MAX_LINE_LEN, "[%s] logged out.%c%c", user.c_str(), 30, 0);
        send(connfd, ret_buf, strlen(ret_buf), 0);
        snprintf(ret_buf, MAX_LINE_LEN, "[%s] leaves the room!%c%c", user.c_str(), 30, 0);
        broadcase(ret_buf, strlen(ret_buf));
        is_logged = false;
    }
}

void ChatServer::help(char *arg, bool &is_looged, int connfd, ChatServer *p_session, std::string &user_name)
{
    char ret_buf[MAX_LINE_LEN];
    snprintf(ret_buf, MAX_LINE_LEN, 
            "supported commands:%c\tlogin <name>%c\tlook%c\tlogout%c\thelp%c\tquit%canything else is to send a message%c%c",
            30, 30, 30, 30, 30, 30, 30, 0
            );
    send(connfd, ret_buf, strlen(ret_buf), 0);
}

void ChatServer::analyse_cmd(char *buf, char *cmd, char *arg, bool is_logged)
{
    if (NULL == buf || NULL == cmd || NULL == arg)
    {
        return ;
    }
    cmd[0] = arg[0] = 0;
    strip(buf);
    DEBUG("buf after strip: %s\n",  buf);
    if (0 == strcasecmp(buf, "help"))
    {
        snprintf(cmd, MAX_LINE_LEN, "%s", buf);
    }
    else if(is_logged && 0 == strcasecmp(buf, "logout"))
    {
        snprintf(cmd, MAX_LINE_LEN, "%s", buf); 
    }
    else if (!is_logged && 0 == strncasecmp(buf, "login ", 6))
    {
        char *p = strchr(buf, ' ');
        if (p)
        {
            *p++ = 0;
            snprintf(arg, MAX_LINE_LEN, "%s", strip(p));
        }
        snprintf(cmd, MAX_LINE_LEN, "%s", buf);
    }
    else if(is_logged && 0 == strcasecmp(buf, "look"))
    {
        snprintf(cmd, MAX_LINE_LEN, "look");
    }
    else
    {
        snprintf(cmd, MAX_LINE_LEN, "say");
        snprintf(arg, MAX_LINE_LEN, "%s", buf);
    }
}


int ChatServer::run()
{
    if (initSock() != 0)
    {
        fprintf(stderr, "initSock failed!\n");
        return 1;
    }
    int connfd = 0;
    int ret = 0;
    int maxi = 0;
    int nfds = 0;
    int i = 0, n = 0;
    char cmd[MAX_LINE_LEN] = "";
    char cmd_arg[MAX_LINE_LEN] = "";
    char line[MAX_LINE_LEN] = "";
    char ret_buf[MAX_LINE_LEN + 100] = "";
    string user_name;
    bool is_logged;
    for(;;)
    {
        nfds = epoll_wait(epfd, events, 20, 500);
        for(i = 0; i < nfds; ++i)
        {
            DEBUG("now nfds[%d]\n", nfds);
            if(events[i].data.fd == listenfd)
            {
                DEBUG("now in accept\n");
                connfd = accept(listenfd, (sockaddr *)&clientaddr, &client);
                if (connfd < 0)
                {
                    fprintf(stderr, "accept failed!\n");
                    continue;
                }
                ret = setnoblocking(connfd);
                if (ret != 0)
                {
                    fprintf(stderr, "setnoblock failed!\n");
                    continue;
                }
                char *str = inet_ntoa(clientaddr.sin_addr);
                DEBUG("connfd[%d], connect from:%s\n", connfd, str);
                snprintf(ret_buf, MAX_LINE_LEN, "welcom to server!%c%c", 30, 0);
                send(connfd, ret_buf, strlen(ret_buf), 0);
                ev.data.fd = connfd;
                ev.events = EPOLLIN | EPOLLET; 
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev); 
                increaseConnect(); 
                addConnfd(connfd); 
            }
            else if(events[i].events & EPOLLIN)
            {
                DEBUG("now in recv\n");
                if ((connfd = events[i].data.fd) < 0)
                {
                    fprintf(stderr, "connfd[%d] error!\n", connfd);
                    continue;
                }
                if ((n = read(connfd, line, MAX_LINE_LEN)) < 0)
                {
                    if (errno == ECONNRESET)
                    {
                        close(connfd);
                        events[i].data.fd = -1;
                    }
                    else
                    {
                        fprintf(stderr, "read line error!\n");
                    }
                }
                else if(0 == n)
                {
                    DEBUG("connfd[%d] exits!\n", connfd);
                    decreaseConnect();
                    removeConnfd(connfd);
                    removeUser(connfd, user_name, is_logged);
                    close(connfd);
                    events[i].data.fd = -1;
                }
                line[n] = 0;
                DEBUG("recieved: %s\n", line);
                ev.data.fd = connfd;
                ev.events = EPOLLOUT | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_MOD, connfd, &ev);
            }
            else if(events[i].events & EPOLLOUT)
            {
                DEBUG("now in send\n");
                connfd = events[i].data.fd;
                analyse_cmd(line, cmd, cmd_arg, is_logged);
                DEBUG("cmd[%s] arg[%s]\n", cmd, cmd_arg);
                std::map<std::string, p_func>::iterator it = m_func.find(cmd);
                if (it != m_func.end())
                {
                    (this->*(it->second))(cmd_arg, is_logged, connfd, this, user_name);
                }
                else
                {
                   snprintf(ret_buf, MAX_LINE_LEN, "cmd error!%c%c", 30, 0);
                   write(connfd, ret_buf, strlen(ret_buf));
                }
                ev.data.fd = connfd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_MOD, connfd, &ev);
            }
        }
    }
    fprintf(stderr, "run finished!\n");
    return 0;
}

int ChatServer::initSock()
{
    int ret = 0;
    epfd = epoll_create(256);
    listenfd  = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == listenfd)
    {
        fprintf(stderr, "create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 1;
    }
    ret = setnoblocking(listenfd);
    if (ret != 0)
    {
        fprintf(stderr, "setnoblocking failed!\n");
        return 1;
    }

    ev.data.fd = listenfd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
    
    //init servaddr
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (-1 == bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)))
    {
        fprintf(stderr, "create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 2;	
    }
    if (-1 == listen(listenfd, 10))
    {
        fprintf(stderr, "listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 3;
    }
    fprintf(stderr, "port: %d\ninitSock finished!\n", port);
    return 0;
}
