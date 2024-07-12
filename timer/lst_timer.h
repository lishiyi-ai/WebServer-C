#ifndef LST_TIMER
#define LST_TIMER
// unistd.h是一个头文件，提供了许多与系统调用和底层操作有关的函数、常量和类型的声明
#include <unistd.h>
// <signal.h> 是 C 标准库中的一个头文件，用于处理信号
#include <signal.h>
// 通过这些常用的原型函数完成编程中对文件的打开、数据写入、数据读取、关闭文件的操作。
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
// epoll是Linux内核为处理大批量文件描述符而作了改进的poll，
// 它能显著提高程序在大量并发连接中只有少量活跃的情况下的系统CPU利用率。
#include <sys/epoll.h>
// 网络协议
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
// sys/mman.h是内存映射（Memory Mapping）的头文件，它提供了用于内存管理的函数
#include <sys/mman.h>
// stdarg 由 standard argument 简化而来，该头文件的主要目的为让函数能够接受可变参数。
#include <stdarg.h>
// 该头文件定义了通过错误码来回报错误资讯的宏。
#include <errno.h>
// 它通常用于等待子进程的状态以及获取子进程的退出状态等操作。
#include <sys/wait.h>
// UIO负责将中断和设备内存暴露给用户空间，然后再由UIO用户态驱动实现具体的业务。
#include <sys/uio.h>

#include <time.h>
#include "../log/log.h"

class util_timer;

struct client_data{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer{
public:
    util_timer(): prev(NULL), next(NULL){}

public:
    time_t expire; // 过期
    void (* cb_func)(client_data *);
    client_data *user_data;
    util_timer *prev;
    util_timer *next;
};

class sort_timer_lst{
public:
    sort_timer_lst();
    virtual ~sort_timer_lst();
    void add_timer(util_timer *timer);
    void adjust_timer(util_timer *timer);
    void del_timer(util_timer *timer);
    void tick();
private:
    void add_timer(util_timer *timer, util_timer *lst_head);
    util_timer *head;
    util_timer *tail;

};

class Utils{
public:
    Utils(){}
    ~Utils(){}

    // 插槽
    void init(int timeslot);

    // 对文件描述符设置非阻塞
    int setnonblocking(int fd);
    // 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGode);
    // 信号处理函数
    static void sig_handler(int sig);
    // 设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);
    // 定时处理任务，重新定时以不断触发sigalrm信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    // 管程文件描述符
    static int *u_pipefd;
    // 网络编成定时器，排序
    sort_timer_lst m_timer_lst;
    // epoll文件描述符
    static int u_epollfd;
    // 时间插槽
    int m_TIMESLOT;

};

void cb_func(client_data *user_data);





#endif