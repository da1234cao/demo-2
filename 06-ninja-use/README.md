
[toc]

## Ninja安装

### windows环境

问题的解决通常有多种方法。按照结果的好坏程度，可以将解决方法简单的划分为，上中下三个层次，见:[为什么谋士总喜欢提上中下三策？
](https://www.zhihu.com/question/37437768)

在windows上安装Ninja, 这里也给出上中下三策。

下策：源码编译。大多数人使用Ninja都不是为了修改它的源码，而是将其作为工具使用。所以，有二进制包，优先使用二进制文件，避免源码编译，编译安装可见：[【ninja】Windows下安装ninja环境](https://blog.csdn.net/qq_43331089/article/details/124479684)

中策：直接从下载二进制包。但是，Ninja这个构建工具，它还依赖rc.exe这样的程序，需要链接kernel32.lib这样的库。在windows powershell使用`cmake -G Ninja`是可能报错的，比如：[Ninja-无法找到rc.exe](https://www.cnblogs.com/zjutzz/p/11669304.html)、[Ninja-无法链接kernel32.lib](https://zhuanlan.zhihu.com/p/540555224)。因为一些程序和库的路径，不在系统的环境变量PATH中。而，在`Developer PowerShell for VS2019`中进行编译则没有问题，因为它已经在PATH中添加了路径，可以通过`$env:path`查看。二进制包安装可见：[How to install ninja-build for C++](https://stackoverflow.com/questions/42710683/how-to-install-ninja-build-for-c)

上策：不用下载。我看了下我安装的VS2019环境中，在安装组件中，将"用于Windows的C++ CMake工具"勾选安装上后，会安装cmake和Ninja。之后，在`Developer PowerShell for VS2019`中进行编译程序，顺顺利利。

### Linux环境

```shell
# ubuntu
sudo apt install ninja-build
```

---

## 入门使用

### 与CMake一起使用

参考： [CMake基础 第10节 使用ninja构建](https://www.cnblogs.com/juzaizai/p/15069678.html) 、[Difference between invoking ninja directly vs through cmake --build](https://stackoverflow.com/questions/70855120/difference-between-invoking-ninja-directly-vs-through-cmake-build)

* 测试ninja代码。

    ```shell
    │  CMakeLists.txt
    │  main.cpp
    ```

    ```cmake
    cmake_minimum_required(VERSION 3.1)
    project(hello_ninja)
    add_executable(${PROJECT_NAME} main.cpp)
    ```

    ```cpp
    #include <iostream>
    int main(int argc, char *argv[]) { std::cout << "hello world" << std::endl; }
    ```

* 编译整个项目

    ```shell
    mkdir build && cd build 
    cmake -G Ninja ..
    ninja
    ```

* 更进一步，我们可以看下生成的`*.ninja`文件。

    ninja文件的相关语法可以参考：[Writing your own Ninja files](https://ninja-build.org/manual.html)。我没有看这个文档，因为没有必要手写ninja文件。看起来和Makefile类似。执行`ninja help`可以看到可以生成的目标。

    ```shell
    ninja.exe help
    [0/1] Re-running CMake...-- Configuring done
    -- Generating done
    -- Build files have been written to: 06-ninja-use/build

    [1/1] All primary targets available:
    edit_cache: phony
    rebuild_cache: phony
    hello_ninja: phony
    all: phony
    build.ninja: RERUN_CMAKE
    clean: CLEAN
    help: HELP
    ```
* 编译的时候添加更多的参数

    ```shell
    # windows上默认生成的是Debug版本,生成Relase版本
    cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release

    # 默认生成的是64位的版本,挺好;把32位的扔到垃圾桶里去
    dumpbin /headers .\hello_ninja.exe
    ```

* 如果需要调试项目。在windows下，直接用vs打开编译生成的可执行文件，即可进行调试。