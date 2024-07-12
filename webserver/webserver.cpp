#include "webserver.h"

WebServer::WebServer(){
    // http_conn类对象
    users = new http_conn[MAX_FD];

    // root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);

    // 定时器
    users_timer = new client_data[MAX_FD];
}

WebServer::~WebServer(){
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete[] m_pool;
}

void WebServer::init(int port, string user, string passWord, 
string databaseName, int log_write, int opt_linger, 
int trigmode, int sql_num,int thread_num, int close_log, int actor_model){
    m_port = port;
    m_user = user;
    m_password = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log = close_log;
    m_actormodel = actor_model;
}

void WebServer::trig_mode(){
    //LT + LT
    if(0 == m_TRIGMode){
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    }else if(1 == m_TRIGMode){ // LT + ET
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    }else if (2 == m_TRIGMode){ //ET + LT
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    }else if (3 == m_TRIGMode){ //ET + ET
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }


}

void WebServer::log_write(){
    if(!m_close_log){
        // 初始化日志
        if(1 == m_log_write){
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 800);
        }else{
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 0);
        }
    }
}

void WebServer::sql_pool(){
    //初始化数据库连接池
    m_connPool = connection_pool::GetInstance();
    m_connPool->init("localhost", m_user, m_password, m_databaseName, 3306, m_sql_num, m_close_log);

    //初始化数据库读取表
    users->initmysql_result(m_connPool);
}

void WebServer::thread_pool(){
    // 线程池
    m_pool = new threadpool<http_conn>(m_actormodel, m_connPool, m_thread_num);
}

void WebServer::eventListen(){
    
    // 网络编程基础步骤
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);
    // 优雅关闭连接
    // 1. l_onoff = 0; l_linger忽略
    // close()立刻返回，底层会将未发送完的数据发送完成后再释放资源，即优雅退出。
    // 2. l_onoff != 0; l_linger = 0;
    // close()立刻返回，但不会发送未发送完成的数据，而是通过一个REST包强制的关闭socket描述符，即强制退出。
    // 3. l_onoff != 0; l_linger > 0;
    // close()不会立刻返回，内核会延迟一段时间，这个时间就由l_linger的值来决定。
    // 如果超时时间到达之前，发送完未发送的数据(包括FIN包)并得到另一端的确认，close()会返回正确，socket描述符优雅性退出。
    // 否则，close()会直接返回错误值，未发送数据丢失，socket描述符被强制性退出。
    // 需要注意的时，如果socket描述符被设置为非堵塞型，则close()会直接返回值。
    if(0 == m_OPT_LINGER){
        struct linger tmp = {0, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }else if(1 == m_OPT_LINGER){
        struct linger tmp = {1, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    int ret = 0;
    struct sockaddr_in address;
    // bzero() 能够将内存块（字符串）的前n个字节清零
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    // SOL_SOCKET:通用套接字选项.
    // SO_REUSEADDR提供如下四个功能：
    // 允许启动一个监听服务器并捆绑其众所周知端口，即使以前建立的将此端口用做他们的本地端口的连接仍存在。
    // 这通常是重启监听服务器时出现，若不设置此选项，则bind时将出错。
    // 允许在同一端口上启动同一服务器的多个实例，只要每个实例捆绑一个不同的本地IP地址即可。
    // 对于TCP，我们根本不可能启动捆绑相同IP地址和相同端口号的多个服务器。
    // 允许单个进程捆绑同一端口到多个套接口上，只要每个捆绑指定不同的本地IP地址即可。这一般不用于TCP服务器。
    // 允许完全重复的捆绑：当一个IP地址和端口绑定到某个套接口上时，还允许此IP地址和端口捆绑到另一个套接口上。
    // 一般来说，这个特性仅在支持多播的系统上才有，而且只对UDP套接口而言（TCP不支持多播）。
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    // 第二个参数backlog为建立好连接处于ESTABLISHED状态的队列的长度
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);
    utils.init(TIMESLOT);

    //epoll创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create(5);
    assert(m_epollfd != 1);
    utils.addfd(m_epollfd, m_listenfd, false, 
    m_LISTENTrigmode);
    http_conn::m_epollfd = m_epollfd;
    //创建管道套接字
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    //设置管道写端为非阻塞，为什么写端要非阻塞？
    utils.setnonblocking(m_pipefd[1]);
    //设置管道读端为ET非阻塞
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);

    //传递给主循环的信号值，这里只关注SIGALRM和SIGTERM
    utils.addsig(SIGPIPE, SIG_IGN);
    utils.addsig(SIGALRM, utils.sig_handler, false);
    utils.addsig(SIGTERM, utils.sig_handler, false);
    alarm(TIMESLOT);
    //工具类,信号和描述符基础操作
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}
void WebServer::timer(int connfd, 
struct sockaddr_in client_address){
    users[connfd].init(connfd, client_address, m_root, 
    m_CONNTrigmode, m_close_log, m_user, m_password,
     m_databaseName);

    // 初始化client_data数据
    // 创建定时器，在设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    util_timer *timer = new util_timer;
    timer->user_data = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    users_timer[connfd].timer = timer;
    utils.m_timer_lst.add_timer(timer);
}

// 若有数据传输，则将定时器往后延迟三个单位
// 并对新的定时器在链表上的位置进行调整
//若有数据传输，则将定时器往后延迟3个单位
//并对新的定时器在链表上的位置进行调整
void WebServer::adjust_timer(util_timer *timer)
{
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);

    LOG_INFO("%s", "adjust timer once");
}
// 执行定时器到的函数，并且删除该定时器
void WebServer::deal_timer(util_timer *timer, int sockfd)
{
    timer->cb_func(&users_timer[sockfd]);
    if (timer)
    {
        utils.m_timer_lst.del_timer(timer);
    }

    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}
bool WebServer::dealclientdata(){
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    // LT
    if(0 == m_LISTENTrigmode){
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        if (connfd < 0){
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if(http_conn::m_user_count >= MAX_FD){
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer(connfd, client_address);

        
    }else{ // ET
        while (1)
        {
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            if (connfd < 0){
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (http_conn::m_user_count >= MAX_FD){
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}
bool WebServer::dealwithsignal(bool &timeout, bool &stop_server){
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    // 出错
    if(ret == -1){
        return false;
    }else if(ret == 0){ // 对方调用了close API来关闭连接
        return false;
    }else{
        for (int i = 0; i < ret; ++i){
            switch (signals[i]){
                // #define SIGALRM  14 由alarm系统调用产生timer时钟信号
                // #define SIGTERM  15 终端发送的终止信号
                case SIGALRM:{
                    timeout = true;
                    break;
                }
                case SIGTERM:{
                    stop_server = true;
                    break;
                }
            }
        }
    }
    return true;
}
void WebServer::dealwithread(int sockfd){
    util_timer *timer = users_timer[sockfd].timer;
    // reacotr
    if(1 == m_actormodel){
        if(timer){
            adjust_timer(timer);
        }

        // 若监测到读事件名将该事件放入请求队列
        m_pool->append(users + sockfd, 0);

        while(true){
            if(1 == users[sockfd].improv){
                if(1 == users[sockfd].timer_flag){
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }

    }else{
        // proactor
        if(users[sockfd].read_once()){
            LOG_INFO("deal with the client(%s)", 
            inet_ntoa(users[sockfd].get_address()->sin_addr));

            //若监测到读事件，将该事件放入请求队列
            m_pool->append_p(users + sockfd);

            if (timer){
                adjust_timer(timer);
            }
        }else {
            deal_timer(timer, sockfd);
        }
    }
}

void WebServer::dealwithwrite(int sockfd){
    util_timer *timer = users_timer[sockfd].timer;
    // reactor
    if(1 == m_actormodel){
        if(timer){
            adjust_timer(timer);
        }
        m_pool->append(users + sockfd, 1);
        while(true){
            if (1 == users[sockfd].improv){
                if (1 == users[sockfd].timer_flag){
                    
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }else{
        // proactor
        if (users[sockfd].write()){
            LOG_INFO("send data to the client(%s)", 
            inet_ntoa(users[sockfd].get_address()->sin_addr));

            if (timer){
                adjust_timer(timer);
            }
        }
        else{
            deal_timer(timer, sockfd);
        }
    }
}

void WebServer::eventLoop(){
    bool timeout = false;
    bool stop_server = false;

    while(!stop_server){
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }
        for(int i = 0; i < number; ++i){
            int sockfd = events[i].data.fd;
            
            //处理新到的客户连接
            if(sockfd == m_listenfd){
                bool flag = dealclientdata();
                if (false == flag)
                    continue;
            }else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                //服务器端关闭连接，移除对应的定时器
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }
            //处理信号
            else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                bool flag = dealwithsignal(timeout, stop_server);
                if (false == flag)
                    LOG_ERROR("%s", "dealclientdata failure");
            }
            // 处理客户连接上受到的数据
            else if (events[i].events & EPOLLIN){
                dealwithread(sockfd);
            }
            else if (events[i].events & EPOLLOUT){
                dealwithwrite(sockfd);
            }
        }
        if(timeout){
            utils.timer_handler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }
}