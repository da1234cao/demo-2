
# libevent入门教程

## 前言

本文是[libevent](https://libevent.org/)的入门文档。本文不涉及，这个库在某些方面的具体使用。

本文内容来自[**A tiny introduction to asynchronous IO**](https://libevent.org/libevent-book/01_intro.html)。通过这篇链接，我们可以了解libevent的基本使用。在开始之前，我们需要一些前置准备：
* TCP客户端和服务端之间的同步通信：[chapter05_TCP客户_服务器示例](https://da1234cao.blog.csdn.net/article/details/105069496)
* IO复用-select的使用：[unix网络编程-select函数](https://da1234cao.blog.csdn.net/article/details/125462501)
* 如果知道这两个就更好了：[epoll实现Reactor模式](https://blog.csdn.net/sinat_38816924/article/details/127706646)、[使用asio实现一个单线程异步的socket服务程序](https://da1234cao.blog.csdn.net/article/details/129339220)

注释：[libevent-文档](https://libevent.org/libevent-book/)、[libevent-API](https://libevent.org/doc/)

---

## libevent-API调用的基本流程

在使用API之前，我们需要了解这三部分：

1. [event_base](https://libevent.org/libevent-book/Ref2_eventbase.html)：`event_base`是一个事件处理的基础结构，它提供了事件循环的基本框架。可以认为它是对select、epoll等函数的封装，以提供跨平台的支持。
2. [event](https://libevent.org/libevent-book/Ref4_event.html)：`event`是一个表示某种事件(套接字的读写事件,定时器事件等)的结构体或对象。它保存了事件触发时，回调函数的指针，供`event_base`调用。事件能被触发的前提时，事件被注册到`event_base`中，参与循环检查。
3. [event loop](https://libevent.org/libevent-book/Ref3_eventloop.html)：运行`event_base`,直到没有任何注册事件。

总的来说：**创建`event`对象后，需要将其添加到一个`event_base`实例中，以便在事件发生时被正确处理。`event_base`提供了方法来添加、删除和处理事件。当事件发生时，`event_base`负责调用与之相关联的回调函数**。下面时一个简单的伪代码示例：

```c
// 创建 event_base 实例
struct event_base *base = event_base_new();

// 创建一个读事件
struct event *ev = event_new(base, fd, EV_READ | EV_PERSIST, callback, arg);

// 将事件添加到事件循环中
event_add(ev, NULL);

// 开始事件循环
event_base_dispatch(base);
```

具体的API接口使用，见官方文档。

下面我们来看一个提供[ROT13](https://zh.wikipedia.org/wiki/ROT13)功能的服务端程序。

```c
// come from: https://libevent.org/libevent-book/01_intro.html

/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <event2/event.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 16384

void do_read(evutil_socket_t fd, short events, void *arg);
void do_write(evutil_socket_t fd, short events, void *arg);

char rot13_char(char c) {
  /* We don't want to use isalpha here; setting the locale would change
   * which characters are considered alphabetical. */
  if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
    return c + 13;
  else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
    return c - 13;
  else
    return c;
}

struct fd_state {
  char buffer[MAX_LINE];
  size_t buffer_used;

  size_t n_written;
  size_t write_upto;

  struct event *read_event;
  struct event *write_event;
};

struct fd_state *alloc_fd_state(struct event_base *base, evutil_socket_t fd) {
  struct fd_state *state = malloc(sizeof(struct fd_state));
  if (!state)
    return NULL;
  state->read_event = event_new(base, fd, EV_READ | EV_PERSIST, do_read, state);
  if (!state->read_event) {
    free(state);
    return NULL;
  }
  state->write_event =
      event_new(base, fd, EV_WRITE | EV_PERSIST, do_write, state);

  if (!state->write_event) {
    event_free(state->read_event);
    free(state);
    return NULL;
  }

  state->buffer_used = state->n_written = state->write_upto = 0;

  assert(state->write_event);
  return state;
}

void free_fd_state(struct fd_state *state) {
  event_free(state->read_event);
  event_free(state->write_event);
  free(state);
}

void do_read(evutil_socket_t fd, short events, void *arg) {
  struct fd_state *state = arg;
  char buf[1024];
  int i;
  ssize_t result;
  while (1) {
    assert(state->write_event);
    result = recv(fd, buf, sizeof(buf), 0);
    if (result <= 0)
      break;

    for (i = 0; i < result; ++i) {
      if (state->buffer_used < sizeof(state->buffer))
        state->buffer[state->buffer_used++] = rot13_char(buf[i]);
      if (buf[i] == '\n') {
        assert(state->write_event);
        event_add(state->write_event, NULL);
        state->write_upto = state->buffer_used;
      }
    }
  }

  if (result == 0) {
    free_fd_state(state);
  } else if (result < 0) {
    if (errno == EAGAIN) // XXXX use evutil macro
      return;
    perror("recv");
    free_fd_state(state);
  }
}

void do_write(evutil_socket_t fd, short events, void *arg) {
  struct fd_state *state = arg;

  while (state->n_written < state->write_upto) {
    ssize_t result = send(fd, state->buffer + state->n_written,
                          state->write_upto - state->n_written, 0);
    if (result < 0) {
      if (errno == EAGAIN) // XXX use evutil macro
        return;
      free_fd_state(state);
      return;
    }
    assert(result != 0);

    state->n_written += result;
  }

  if (state->n_written == state->buffer_used)
    state->n_written = state->write_upto = state->buffer_used = 0;

  event_del(state->write_event);
}

void do_accept(evutil_socket_t listener, short event, void *arg) {
  struct event_base *base = arg;
  struct sockaddr_storage ss;
  socklen_t slen = sizeof(ss);
  int fd = accept(listener, (struct sockaddr *)&ss, &slen);
  if (fd < 0) { // XXXX eagain??
    perror("accept");
  } else if (fd > FD_SETSIZE) {
    close(fd); // XXX replace all closes with EVUTIL_CLOSESOCKET */
  } else {
    struct fd_state *state;
    evutil_make_socket_nonblocking(fd);
    state = alloc_fd_state(base, fd);
    assert(state); /*XXX err*/
    assert(state->write_event);
    event_add(state->read_event, NULL);
  }
}

void run(void) {
  evutil_socket_t listener;
  struct sockaddr_in sin;
  struct event_base *base;
  struct event *listener_event;

  base = event_base_new();
  if (!base)
    return; /*XXXerr*/

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(40713);

  listener = socket(AF_INET, SOCK_STREAM, 0);
  evutil_make_socket_nonblocking(listener);

  // not work in windows system
  evutil_make_listen_socket_reuseable(listener);

  if (bind(listener, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    perror("bind");
    return;
  }

  if (listen(listener, 16) < 0) {
    perror("listen");
    return;
  }

  listener_event =
      event_new(base, listener, EV_READ | EV_PERSIST, do_accept, (void *)base);
  /*XXX check it */
  event_add(listener_event, NULL);

  event_base_dispatch(base);
}

int main(int c, char **v) {
  setvbuf(stdout, NULL, _IONBF, 0);

  run();
  return 0;
}
```

---

## 给事件提供缓冲区

上面的代码中，我们需要为每个连接显示的分配堆栈作为缓冲区，这是麻烦的。

网络异步编程的通常模式是：
1. 当连接可读时，将数据读取到缓冲区，然后处理。
2. 将处理结果写入输出缓冲区，然后将连接设置为可写。尽可能多地写入数据。
3. 写完之后，关闭连接的可写。(因为套接字的输出缓冲区不满的时候，总是可写的。)

Libevent 提供了一个 它的通用机制：[Bufferevents](https://libevent.org/libevent-book/Ref6_bufferevent.html)。它包含一个底层传输(如套接字)、读缓冲区和写缓冲区缓冲。从底层IO中读取数据到用户层的缓冲区，然后触发读取回调；将需要发送的数据，放入用户层的缓冲区中，libevent会将数据发送出去。每个Bufferevents都有下面四个watermarks：
* Read low-water mark: 当输入缓冲区存储的数据，比当前设置的低水位多，则调用读取回调。
* Read high-water mark：当输入缓冲区的数据，达到当前的高水位，缓冲区不再从底层IO去获取更多数据。等待回调函数消耗些数据，缓冲区才有空间继续存放从IO获取来数据。
* Write low-water mark：当输出缓冲区存储的数据，比当前设置的低水位多，则调用写回调。
* Write high-water mark：可以暂时不管。具体见官方文档。

下面使用bufferevent，重写上一节的代码：

```c
/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 16384

void do_read(evutil_socket_t fd, short events, void *arg);
void do_write(evutil_socket_t fd, short events, void *arg);

int print_output(struct bufferevent *bev) {
  struct evbuffer *output = bufferevent_get_output(bev);
  size_t len = evbuffer_get_length(output);
  char buff[len + 1];
  int size = evbuffer_copyout(output, buff, len);
  if (size < 0) {
    printf("read fail\n");
    return -1;
  }
  buff[size] = '\0';
  printf("Output Buffer Contents: %s\n", buff);
  return 0;
}

char rot13_char(char c) {
  /* We don't want to use isalpha here; setting the locale would change
   * which characters are considered alphabetical. */
  if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
    return c + 13;
  else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
    return c - 13;
  else
    return c;
}

void readcb(struct bufferevent *bev, void *ctx) {
  struct evbuffer *input, *output;
  char *line;
  size_t n;
  int i;
  input = bufferevent_get_input(bev);
  output = bufferevent_get_output(bev);

  while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF))) {
    for (i = 0; i < n; ++i)
      line[i] = rot13_char(line[i]);
    evbuffer_add(output, line, n);
    evbuffer_add(output, "\n", 1);
    free(line);
  }

  if (evbuffer_get_length(input) >= MAX_LINE) {
    /* Too long; just process what there is and go on so that the buffer
     * doesn't grow infinitely long. */
    char buf[1024];
    while (evbuffer_get_length(input)) {
      int n = evbuffer_remove(input, buf, sizeof(buf));
      for (i = 0; i < n; ++i)
        buf[i] = rot13_char(buf[i]);
      evbuffer_add(output, buf, n);
    }
    evbuffer_add(output, "\n", 1);
  }
}

void errorcb(struct bufferevent *bev, short error, void *ctx) {
  if (error & BEV_EVENT_EOF) {
    /* connection has been closed, do any clean up here */
    /* ... */
  } else if (error & BEV_EVENT_ERROR) {
    /* check errno to see what error occurred */
    /* ... */
  } else if (error & BEV_EVENT_TIMEOUT) {
    /* must be a timeout event handle, handle it */
    /* ... */
  }
  bufferevent_free(bev);
}

void do_accept(evutil_socket_t listener, short event, void *arg) {
  struct event_base *base = arg;
  struct sockaddr_storage ss;
  socklen_t slen = sizeof(ss);
  int fd = accept(listener, (struct sockaddr *)&ss, &slen);
  if (fd < 0) {
    perror("accept");
  } else if (fd > FD_SETSIZE) {
    close(fd);
  } else {
    struct bufferevent *bev;
    evutil_make_socket_nonblocking(fd);
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);
    bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
  }
}

void run(void) {
  evutil_socket_t listener;
  struct sockaddr_in sin;
  struct event_base *base;
  struct event *listener_event;

  base = event_base_new();
  if (!base)
    return; /*XXXerr*/

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(40713);

  listener = socket(AF_INET, SOCK_STREAM, 0);
  evutil_make_socket_nonblocking(listener);

#ifndef WIN32
  {
    int one = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  }
#endif

  if (bind(listener, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    perror("bind");
    return;
  }

  if (listen(listener, 16) < 0) {
    perror("listen");
    return;
  }

  listener_event =
      event_new(base, listener, EV_READ | EV_PERSIST, do_accept, (void *)base);
  /*XXX check it */
  event_add(listener_event, NULL);

  event_base_dispatch(base);
}

int main(int c, char **v) {
  setvbuf(stdout, NULL, _IONBF, 0);

  run();
  return 0;
}
```