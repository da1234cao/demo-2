[toc]

## grpc的安装

Linux

```shell
sudo apt -y install protobuf-compiler-grpc
sudo apt install libgrpc++-dev
```


```shell
protoc -I .. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` --cpp_out=. calculator.proto
```