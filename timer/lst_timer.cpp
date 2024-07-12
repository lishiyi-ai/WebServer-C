#include "lst_timer.h"
#include "../http/http_conn.h"

// 采用双向链表，按照从小到大进行排序
sort_timer_lst::sort_timer_lst(){
    head = NULL;
    tail = NULL;
}
sort_timer_lst::~sort_timer_lst(){
    util_timer *tmp = head;
    while(tmp){
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer){
    if(!timer){
        return;
    }
    if(!head){
        head = tail = timer;
        return;
    }
    // 头部插入
    if(timer->expire < head->expire){
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);
}

void sort_timer_lst::add_timer(util_timer *timer, 
util_timer *lst_head){
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    // 链表中间插入
    while(tmp){
        if(timer->expire < tmp->expire){
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }

    if(!tmp){
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}
// 当定时任务发生变化,调整对应定时器在链表中的位置
void sort_timer_lst::adjust_timer(util_timer *timer){
    if(!timer){
        return;
    }
    util_timer *tmp = timer->next;
    if(!tmp || (timer->expire < tmp->expire)){
        return;
    }
    // 头部取出
    if(timer == head){
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }else{ // 中间取出
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

void sort_timer_lst::del_timer(util_timer *timer){
    if(!timer){
        return;
    }

    if(timer == head && timer == tail){
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }

    if(timer == head){
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }

    if(timer == tail){
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }

    timer->next->prev = timer->prev;
    timer->prev->next = timer->next;
    delete timer;
}
// 定时函数处理任务
void sort_timer_lst::tick(){
    if(!head){
        return;
    }
    time_t cur = time(NULL);
    util_timer *tmp = head;
    while(tmp){
        if(cur < tmp->expire){
            break;
        }
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head)
        {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}

void Utils::init(int timeslot){
    m_TIMESLOT = timeslot;
}
// 对文件描述符设置非阻塞
int Utils::setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
// 将内核事件在表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, 
bool one_shot,int TRIGMode){
    epoll_event event;
    event.data.fd = fd;
    
    if(1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;
    if(one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

// 信号处理函数
void Utils::sig_handler(int sig){
    //为保证函数的可重入性，保留原来的errno
    //可重入性表示中断后再次进入该函数，环境变量与之前相同，不会丢失数据
    int save_errno = errno;
    int msg = sig;
    //将信号值从管道写端写入，传输字符类型，而非整型
    send(u_pipefd[1], (char *)&msg, 1, 0);
    //将原来的errno赋值为当前的errno
    errno = save_errno;
}
// 设置信号函数 // 修改
void Utils::addsig(int sig, void(*handler)(int)/*函数指针*/, 
bool restart){
    //创建sigaction结构体变量
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    //信号处理函数中仅仅发送信号值，不做对应逻辑处理
    sa.sa_handler = handler;
    if(restart)
        sa.sa_flags |= SA_RESTART;
    // sigfillset()函数用于初始化一个自定义信号集，将其所有信号都填充满，
    // 也就是将信号集中的所有的标志位置为1，
    // 使得这个集合包含所有可接受的信号，也就是阻塞所有信号。
    sigfillset(&sa.sa_mask);
    //执行sigaction函数
    assert(sigaction(sig, &sa, NULL) != -1);
}
// 定时处理任务，重新定时以不断触发sigalrm信号
void Utils::timer_handler(){
    m_timer_lst.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info){
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data){
    epoll_ctl(Utils::u_epollfd, 
    EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data->sockfd);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}
