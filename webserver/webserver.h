#ifndef WEBSERVER_H
#define WEBSERVER_h

// 网络编程
#include <sys/socket.h>
// 网络相关的数据结构
#include <netinet/in.h>
// ARP解析协议
#include <arpa/inet.h>
#include <stdio.h>
// 对 POSIX 操作系统 API 的访问功能的头文件的名称
#include <unistd.h>
// 指示在程序运行过程中发生的错误
#include <errno.h>
// 完成编程中对文件的打开、数据写入、数据读取、关闭文件的操作
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
// epollepoll是linux内核实现IO多路复用的一个实现，
// 可以同时监听多个输入输出源，
// 在其中一个或多个输入输出源可用的时候返回，然后对其进行读写操作
#include <sys/epoll.h>
#include "../CGImysql/sql_connection_pool.h"
#include "../http/http_conn.h"
#include "../threadpool/threadpool.h"

const int MAX_FD = 65536; // 最大文件描述符
const int MAX_EVENT_NUMBER = 10000; // 最大事件数
const int TIMESLOT = 5; // 最小超时单位

class WebServer{
public:
    WebServer();
    ~WebServer();
    void init(int port, string user, string passWord, string databaseName,
    int log_write, int opt_linger, int trigmode, int sql_num, int thread_num,
    int close_log, int actor_model);

    void thread_pool();
    void sql_pool();
    void log_write();
    void trig_mode();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void deal_timer(util_timer *timer, int sockfd);
    bool dealclientdata();
    bool dealwithsignal(bool& timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);


private:
    // 基础
    int m_port;
    char *m_root;
    int m_log_write;
    int m_close_log;
    int m_actormodel;
    int m_pipefd[2];
    int m_epollfd;

    http_conn *users;

    // 数据库相关
    connection_pool *m_connPool;
    string m_user; // 登陆数据库用户名
    string m_password; // 三登陆数据库密码
    string m_databaseName; // 使用数据库名
    int m_sql_num;

    // 线程池相关
    threadpool<http_conn> *m_pool;
    int m_thread_num;
    
    // epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];
    
    int m_listenfd;
    int m_OPT_LINGER;
    int m_TRIGMode;
    int m_LISTENTrigmode;
    int m_CONNTrigmode;

    // 定时器相关
    client_data *users_timer;
    Utils utils;
};
#endif