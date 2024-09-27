# WebServer-C
使用数据库连接池管理数据连接

使用RAII对每个数据库的存还进行管理，通过C++类来保证其安全的存取。

使用线程池进行线程管理，使用同步I/O模拟proactor模式

锁类封装了互斥锁，信号量，条件变量

http类采用主从状态机进行，从状态机负责读取行，主状态机负责解析行，主状态机调用从状态机，从状态机驱动主状态机

日志类实现了同步 和 异步模式，异步模式采用阻塞队列进行管理。

实现按天、超行分类。
webserver类中支持react模式 和 proactor模式
代码中在数据库连接池与日志类中采用了单例模式中的懒汉模式，C++11之后保证了懒汉模式的线程安全。
定时器类采用了双端链表进行定时器管理，采用信号量触发定时任务，主要处理非活动连接。

-服务器测试环境
  -Ubuntu版本24.04
  -MySQL版本8.0.34
-浏览器测试环境
  -Window、Linux均可
  -FireFox
  -Chrome
  -Edge
-数据库
~~~
// 建立webuserdb库
create database webuserdb;

// 创建user表
USE yourdb;
CREATE TABLE user(
    username char(50) NULL,
    passwd char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, passwd) VALUES('name', 'passwd');
~~~

* -p，自定义端口号
  * 默认9000
* -l，选择日志写入方式，默认同步写入
  * 0，同步写入
  * 1，异步写入
* -m，listenfd和connfd的模式组合，默认使用LT + LT
  * 0，表示使用LT + LT
  * 1，表示使用LT + ET
  * 2，表示使用ET + LT
  * 3，表示使用ET + ET
* -o，优雅关闭连接，默认不使用
  * 0，不使用
  * 1，使用
* -s，数据库连接数量
  * 默认为8
* -t，线程数量
  * 默认为8
* -c，关闭日志，默认打开
  * 0，打开日志
  * 1，关闭日志
* -a，选择反应堆模型，默认Proactor
  * 0，Proactor模型
  * 1，Reactor模型
 
参考项目地址：https://github.com/qinguoyi/TinyWebServer
