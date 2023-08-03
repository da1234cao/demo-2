package server

// tips1: 包中需要导出的内容,需要大写字母开头

import (
	"bufio"
	"fmt"
	"net"
	"strconv"
)

type Server interface {
	start()
}

type TcpServer struct {
	Ip   string
	Port int
}

func (server *TcpServer) start() {
	fmt.Println("tcp server start...")

	// 创建 listener
	listener, err := net.Listen("tpc", server.Ip+":"+strconv.Itoa(server.Port))
	if err != nil {
		fmt.Println("Error listening:", err.Error())
		return
	}

	// 监听并接受来自客户端的连接
	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println("Error accepting", err.Error())
			return
		}
		go tcpConnProcess(conn)
	}
}

func tcpConnProcess(conn net.Conn) {
	for {
		reader := bufio.NewReader(conn)
	}
}
