#pragma once

/**
 * 一般是不建议在头文件中定义变量的,(没有static)会导致变量重复定义。
 * 这里加上static,每个引入的文件,会创建自己的signal_name,导致创建多个
 * 这样写比较省事,虽然语法编译上可以通过,但是和实际想表达的全局变量不同
 * https://blog.csdn.net/weibo1230123/article/details/83000786
 */
#define EVENT_SIZE 2
static const char *events_name[] = {"_test_event_one_", "_test_event_two_"};
