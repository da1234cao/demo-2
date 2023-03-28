# C++11使用using定义别名（替代typedef）

## 把typedef丢进垃圾桶里

`typedef`用来给类型定义别名。从C++11开始，`using`可以用来给类型定义别名，它完全可以替代`typedef`。

`using`除了提供类型别名的功能，还可以通过别名模版指代一族类型的名字。

关于`using`的介绍，可以参考下面连接：

* [C++ 关键词：using](https://zh.cppreference.com/w/cpp/keyword/using) -- using功能的全面介绍
* [What is the logic behind the "using" keyword in C++?](https://stackoverflow.com/questions/20790932/what-is-the-logic-behind-the-using-keyword-in-c) -- C++添加了新功能，但是没有引入新的关键字
* [C++11使用using定义别名（替代typedef）](http://c.biancheng.net/view/3730.html) -- using的使用

下面是一个使用`using`的demo。以后(c++)编程，需要给类型定义别名，统统使用`using`。

```cpp
#include <iostream>

template<typename T>
using call_back = void (*) (T);

void print_int(int num) {std::cout << num << std::endl;}

int main(int argc, char* argv[]){
    call_back<int> func = print_int;
    func(233);
    return 0;
}
```

---

## 查看当前编译器支持的C/C++标准

`using`的类型别名功能，从C++11标准才开始。所以，我们需要查看当前编译器支持的C/C++标准。

即使编译器支持了需要的C++标准，但也可能只是支持部分功能。C++编译器支持情况，可以参考：[C++ 编译器支持情况表](https://c-cpp.com/cpp/compiler_support)。

至于查看当前环境的C++标准支持。

* MSVC的C++标准查看方法：待。

* gcc的C++标准查看方法：`g++ -dM -E - < /dev/null | grep -e __cplusplus -e __STDC_VERSION__`

* clang的C++标准查看方法：`clang -dM -E - < /dev/null | grep -e __cplusplus -e __STDC_VERSION__`

上面命令的含义，我问了下chat-gpt，参数大体如下：
* -dM：指示g++输出预定义宏的列表。(-d后面可以接不同的字母选项，如-da,-db等)
* -E：指示g++对输入文件进行预处理，并将预处理输出写入标准输出。因为输入文件是从标准输入（即键盘）读取的，所以这里使用-表示输入文件是从管道中读取的。
* < /dev/null：将/dev/null设备文件的内容作为标准输入传递给g++，以便不必输入任何内容。

我当前的环境支持到c++17。所以C++11的标准放心用。当然，最好是在编译中添加C++标准参数：

```cmake
# cmake 
set(CMAKE_CXX_STANDARD 11)
```