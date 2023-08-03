package main

import (
	"fmt"
	"go-tcp-demo/server"
)

func main() {
	s := server.TcpServer{Ip: "127.0.0.1", Port: 10000}
	fmt.Println(s.Ip)
}
