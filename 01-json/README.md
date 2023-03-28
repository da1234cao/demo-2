## 前言

json-wiki

c++有很多json库：


## 

### 

查看[Boost Version History](https://www.boost.org/users/history/)，我们知道[boost 1.75.0](https://www.boost.org/users/history/version_1_75_0.html)中引入Json库。

[Boost.JSON](https://www.boost.org/doc/libs/1_79_0/libs/json/doc/html/index.html)

从上向下，从内向外。
* 从上向下：遇到花括号，创建一个object。遇到中括号，创建一个array。
* 从内向外：遇到嵌套结构，先构造里面内容，里面构造完后，赋值改外层。

### 

[【C++ JSON 开源库】nlohmann入门使用总结 ](https://www.cnblogs.com/linuxAndMcu/p/14503341.html)