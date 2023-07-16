
## 安装

```shell
# 查找go的包
winget.exe search golang
...
Go Programming Language GoLang.Go                   1.20.6   Tag: golang winget
...

# 安装
winget.exe install -e --id GoLang.Go --version 1.20.6

go.exe version
go version go1.20.6 windows/amd64

# 可以看到环境当前用户的PATH环境变量中多了一条
%USERPROFILE%\go\bin

# 系统的环境变量中也多了一条
C:\Program Files\Go\bin
```

## hello world

```shell
go mod init main

go mod tidy

go mod edit -replace example.com/greetings=../greetings
```