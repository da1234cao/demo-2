package main

import (
	"go-tcp-demo/server"
)

func main() {
	s := server.TcpServer{Ip: "127.0.0.1", Port: 10000}
	s.Start()
}
