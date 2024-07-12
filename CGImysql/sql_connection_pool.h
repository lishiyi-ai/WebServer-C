#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"
#include "../log/log.h"

using namespace std;
class connection_pool{
public:
    MYSQL *GetConnection(); // 获取数据库连接
    bool ReleaseConnection(MYSQL *conn); // 释放连接
    int GetFreeConn(); // 获取空闲连接
    void DestroyPool(); // 销毁所有连接

    // 单例模式 饿汉模式
    static connection_pool *GetInstance(); 

    // 初始化函数
    void init(string url, string User, string Pasword , string DataBaseName, 
            int Port, int MaxConnm, int close_log);

private:
    connection_pool();
    virtual ~connection_pool();

    int m_MaxConn; // 最大连接数
    int m_CurConn; // 当前已经使用的连接数
    int m_FreeConn; // 当前空闲的连接数
    locker lock; // 锁
    list<MYSQL *> connList; // 连接池
    sem reserve; // 信号量 表示可用空闲连接实现锁机制

// public:
private:
    string m_url; // 主机地址
    int m_Port; // 数据库端口号
    string m_User; // 数据库名称
    string m_Password; // 数据库密码
    string m_DataBaseName; // 数据库名
    int m_close_log; // 日志开关
};

// RAII是C++中一种管理资源、避免资源泄漏的惯用法，利用栈对象自动销毁的特点来实现。
class connectionRAII{

public:
    connectionRAII(MYSQL **con, connection_pool *connPool);
    virtual ~connectionRAII();

private:
    MYSQL *conRAII;
    connection_pool *poolRAII;
};

#endif