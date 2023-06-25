[toc]

## 前言

## probuf安装

Window上安装probuf。首先需要配置好`vcpkg.exe`。可以参考：[vcpkg安装](https://blog.csdn.net/sinat_38816924/article/details/131360952#t5)，或者网上其他更加相信教程。

```shell
vcpkg.exe install protobuf
```

Linux上安装probuf

```shell
sudo apt install libprotobuf-dev protobuf-compiler
```

## protobuf在c++中的简单使用

官方文档：[Protocol Buffer Basics: C++](https://protobuf.dev/getting-started/cpptutorial/) -- 官方的示例挺好了

如果需要在cmake中使用，见：[FindProtobuf](https://cmake.org/cmake/help/latest/module/FindProtobuf.html)、[CMake with Google Protocol Buffers](https://stackoverflow.com/questions/20824194/cmake-with-google-protocol-buffers)

基本逻辑是：编写`.proto`文件，以定义消息格式；使用`protoc`将`.proto`文件转换成指定语言的类接口；使用 C++ protocol buffer API 读写消息；

具体的细节，见官方文档。