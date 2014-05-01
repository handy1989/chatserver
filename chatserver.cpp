#include "strtools.h"
#include "chatserver.h"

using std::vector;
using std::string;

int ChatServer::hasUser(const string &name)
{
	return s_users.count(name); 
}

void ChatServer::addUser(int connfd, const string &name)
{
    m_users[connfd] = name;
    s_users.insert(name);
}

void ChatServer::removeUser(int connfd, const string &name)
{
    m_users.erase(connfd);
    s_users.erase(name);
}

void ChatServer::destroy_connfd(int connfd_index)
{
    if (connfd_index < MAX_THREAD_NUM)
    {
        connfd_arr[connfd_index] = -1;
    }
}

int ChatServer::get_connfd(int connfd_index)
{
    return connfd_index < MAX_THREAD_NUM ? connfd_arr[connfd_index] : -1;
}

int ChatServer::get_valid_connfd_index()
{
    int i = 0;
    for(i = 0; i < MAX_THREAD_NUM; ++i)
    {
        if (get_connfd(i) < 0)
        {
            break;
        }
    }
    return i;
}

void ChatServer::login(char *arg, bool &is_logged, int connfd, ChatServer *p_session, std::string &user_name)
{
    char ret_buf[MAX_LINE_LEN];
    //send(connfd, "you're logging", 100, 0);
    DEBUG("log name[%s]", arg);
    if (is_logged)
    {
        say(arg, is_logged, connfd, p_session, user_name);
        return ;
    }
    if (hasUser(arg))
    {
        snprintf(ret_buf, MAX_LINE_LEN, "User[%s] already exits. Please try again.%c", arg, 0);
        send(connfd, ret_buf, MAX_LINE_LEN, 0);
    }
    else
    {
        addUser(connfd, arg);
        user_name = arg;
        snprintf(ret_buf, MAX_LINE_LEN, "%s joins the room!%c", arg, 0);
        broadcase(ret_buf);
        is_logged = true;
    }
    DEBUG("ret_buf[%s]", ret_buf);
    
}

void ChatServer::broadcase(char *msg)
{
    int ret = 0;
    if (NULL == msg)
    {
        return ;
    }
    for(int i = 0; i < MAX_THREAD_NUM; ++i)
    {
        if (m_users.count(get_connfd(i)) > 0)
        {
            ret = send(connfd_arr[i], msg, strlen(msg), 0);
            if (ret < 0)
            {
                fprintf(stderr, "send recieved message failed!\n");
            } 	
        }	
    }
}

void ChatServer::look(char *arg, bool &is_logged, int connfd, ChatServer *p_session, std::string &user_name)
{
    DEBUG("look\n");
    char ret_buf[MAX_LINE_LEN];
    if (!is_logged)
    {    
        snprintf(ret_buf, MAX_LINE_LEN, "You haven't been logged in. Please login first.%c", 0);
        int len = strlen(ret_buf);
        DEBUG("ret_buf[%s] len[%d]\n", ret_buf, len);
        send(connfd, ret_buf, strlen(ret_buf), 0);
    }
    else
    {
        string str_ret_buf("user list:");
        std::set<std::string>::iterator it;
        for (it = s_users.begin(); it != s_users.end(); ++it)
        {
            str_ret_buf = str_ret_buf + "\n" + *it;
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
        snprintf(ret_buf, MAX_LINE_LEN, "You haven't been logged in. Please login first.%c", 0);
        int len = strlen(ret_buf);
        DEBUG("ret_buf[%s] len[%d]\n", ret_buf, len);
        send(connfd, ret_buf, strlen(ret_buf), 0);
    }
    else
    {
        std::string user_name = m_users[connfd];
        snprintf(ret_buf, MAX_LINE_LEN, "[%s]%s%c", user_name.c_str(), arg, 0);
        DEBUG("ret_buf[%s]\n", ret_buf);
        p_session->broadcase(ret_buf);
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
        removeUser(connfd, user);
        snprintf(ret_buf, MAX_LINE_LEN, "[%s] logged out.%c", user.c_str(), 0);
        send(connfd, ret_buf, strlen(ret_buf), 0);
        snprintf(ret_buf, MAX_LINE_LEN, "[%s] leaves the room!%c", user.c_str(), 0);
        broadcase(ret_buf);
        is_logged = false;
    }
}

void ChatServer::analyse_cmd(char *buf, char *cmd, char *arg, bool is_logged)
{
    if (NULL == buf || NULL == cmd || NULL == arg)
    {
        return ;
    }
    int cmd_len = sizeof(cmd);
    int arg_len = sizeof(arg);
    cmd[0] = arg[0] = 0;
    strip(buf);
    if(is_logged && 0 == strcasecmp(buf, "logout"))
    {
        snprintf(cmd, cmd_len, "%s", buf); 
    }
    else if (!is_logged && 0 == strncasecmp(buf, "login ", 6))
    {
        char *p = strchr(buf, ' ');
        if (p)
        {
            *p++ = 0;
            snprintf(arg, arg_len, "%s", strip(p));
        }
        snprintf(cmd, cmd_len, "%s", buf);
    }
    else if(is_logged && 0 == strcasecmp(buf, "look"))
    {
        snprintf(cmd, cmd_len, "look");
    }
    else
    {
        snprintf(cmd, cmd_len, "say");
        snprintf(arg, arg_len, "%s", buf);
    }
}

void *ChatServer::talk_thread(void *arg)
{
    thread_para_t *thread_para = static_cast<thread_para_t *>(arg);
	ChatServer *p_session = thread_para->p_session;
    int connfd_index = thread_para->connfd_index;
    int connfd = p_session->connfd_arr[connfd_index];

    p_session->increase_thread();
	char buff[MAX_LINE_LEN];
	char ret_buf[MAX_LINE_LEN + 50];
	int msg_len = 0;
	int ret = 0;
	ret = send(connfd, "welcom to server!", 100, 0);
	if (ret < 0)
	{
		fprintf(stderr, "send welcom failed!\n");
		p_session->destroy_connfd(connfd_index);
        p_session->decrease_thread();
        close(connfd);
        delete thread_para;
		return (void *)1;	
	}
    bool is_logged = false;
    char cmd[MAX_LINE_LEN] = "";
    char cmd_arg[MAX_LINE_LEN] = "";
    string user_name;
    while (true)
    {
        msg_len = recv(connfd, buff, MAX_LINE_LEN, 0);
        if (0 == msg_len)
        {
            fprintf(stderr, "recieved from client failed!\n");
            break;
        }
        buff[msg_len] = 0;
        printf("connfd_arr[%d]=[%d] recieved: %s\n", connfd_index, connfd, buff);
        p_session->analyse_cmd(buff, cmd, cmd_arg, is_logged);
        printf("cmd[%s] arg[%s]\n", cmd, cmd_arg);
        std::map<std::string, p_func>::iterator it = p_session->m_func.find(cmd);
        if (it != p_session->m_func.end())
        {
            (p_session->*(it->second))(cmd_arg, is_logged, connfd, p_session, user_name);
        }
        else
        {
            send(connfd, "cmd error!", 100, 0);
        }
    }
	close(connfd);
    p_session->removeUser(connfd, user_name);
	p_session->destroy_connfd(connfd_index);
    p_session->decrease_thread();
    delete thread_para;
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
    thread_para_t *thread_para;
	while (true)
	{
		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
		if (-1 == connfd)
		{
			fprintf(stderr, "accept socket error: %s(errno: %d)\n", strerror(errno), errno);
			continue;
		}		
		int connfd_index = get_valid_connfd_index();
		if (connfd_index >= MAX_THREAD_NUM)
		{
			fprintf(stderr, "Too many threads! No larger than %d. Please wait.\n", MAX_THREAD_NUM);
            char tmp_buf[MAX_LINE_LEN];
            snprintf(tmp_buf, MAX_LINE_LEN, "Room is full. No more than %d people. Please wait for a moment", MAX_THREAD_NUM);
	        send(connfd, tmp_buf, MAX_LINE_LEN, 0);
			close(connfd);
			continue;
		}
        thread_para = new thread_para_t;
        thread_para->p_session = this;
        thread_para->connfd_index = connfd_index;
		DEBUG("in function run connfd_index[%d] connfd[%d]\n", connfd_index, connfd);
		connfd_arr[connfd_index] = connfd;
		ret = pthread_create(&thread[connfd_index], NULL, talk_thread, thread_para);
		if (0 != ret)
		{
				fprintf(stderr, "create thread failed!\n");
                destroy_connfd(connfd_index);
                delete thread_para;
				close(connfd);
		}
		else
		{
			fprintf(stderr, "create thread[%d] success!\n", cur_thread_num);
		}
	}
	fprintf(stderr, "run finished!\n");
	return 0;
}

int ChatServer::initSock()
{
	int ret = 0;
	listenfd  = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == listenfd)
	{
		fprintf(stderr, "create socket error: %s(errno: %d)\n", strerror(errno), errno);
		return 1;
	}
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
