# ReactorHttpServer
A simple Http-Server based on Reactor model for handling service http-requests delivered concurrently to a service handler by one or more inputs.

本项目是一个高并发网络服务器模型，该模型基于事件驱动的Reactor多反应堆模式，使用I/O多路复用技术，且在服务器端提供了多线程来处理任务，并使用线程池对子线程进行管理，通过C++语言进行实现，在应用层使用了Http协议，可处理简单的浏览器请求。

### 项目主要模块

#### 反应堆模型

- MainEventLoop：主反应堆，绑定本地IP和端口，获取ListenFD并监听其读事件，创建线程池
- EventLoop：事件循环，用于程序中所有事件的处理。
- Listener：监听器，监听端口和监听的FD。
- Dispatcher：分发器，函数指针指定I/O多路复用模型（select/poll/epoll），及对FD事件触发后的具体处理操作。
- Channel：封装了FD，检测FD的哪些事件，以及当该FD对应事件触发后调用的（读/写/删除）回调函数。
- ChannelMap：存储fd和channel的映射关系。
- TaskQueue：任务队列，记录需要进行处理的FD及其事件，每个任务都是一个channel实例。

#### 服务器模块

- TcpServer：建立线程池和事件循环，监听ListenFD，当触发读事件时，和客户端建立连接，将ConnectFD分发给子线程进行处理。

#### I/O模型

- Buffer： Input_Buffer读缓冲区、Output_Buffer写缓冲区。
- TcpConnection：封装了子线程的EventLoop、每个ConnectFD的Channel。

#### Http功能模块

- HttpRequest：处理解析Http请求，保存请求行和请求头。
- HttpResponse：组织并发送Http响应报文。

#### 多线程模块

- ThreadPool：线程池管理子线程。
- WorkerThread：工作的子线程，负责和对应客户端进行通信。使用获得的ConnectFD，封装一个TcpConnection，放入子线程的EventLoop反应堆进行处理，每个子线程都有一个反应堆模型，当ConnectFD的事件（读/写）触发之后，子线程反应堆模型调用对应的处理函数，对事件进行相应处理。


### 项目工作流程

1. 主线程绑定本地IP和端口，获取ListenFD并监听其读事件，若读事件激活，则将读事件注册给主线程的Reactor反应堆的IO复用模型（select/poll/epoll）。
2. 在主线程中，通过Dispatcher分发器，对所有FD进行处理（添加/删除/修改读写事件），建立TaskQueue任务队列，主反应堆中遍历处理任务，修改IO复用模型FD相关的事件。
3. 调用accept函数与客户端建立新连接，获得用于通信的ConnectFD。封装ConnectFD记录其事件类型，同时指定触发事件（读/写）的回调函数，建立连接之后创建线程池，将ConnectFD对应的事件注册到线程池中某个子线程的反应堆模型中。
4. 子线程实现与客户端的通信，接收及解析Http请求数据，组织并发送Http响应报文，管理读写缓冲区。


### webbench压力测试

测试环境

- CPU：2*2核心，i5-6200U，2.3GHz
- 内存：2G
- Linux：UbuntuV22.04，64位服务器版，内核5.15.0
- 测试软件：Webbench v1.5

```shell
:~# webbench -c 4000 -t 60 http://192.168.111.128:9999/
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://192.168.111.128:9999/
4000 clients, running 60 sec.
Speed=40782 pages/min, 17249822 bytes/sec.
Requests: 40548 susceed, 234 failed.
```

TPS每秒事务数：600+

