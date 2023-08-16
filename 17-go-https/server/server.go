package server

import (
	"bufio"
	"bytes"
	"crypto/tls"
	"errors"
	"io"
	"log"
	"net"
	"net/http"
	"strconv"
)

func reflect(w http.ResponseWriter, r *http.Request) {
	log.Println("handle reflect")
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.WriteHeader(http.StatusOK)
	bodyByte, _ := io.ReadAll(r.Body)
	log.Println("recv:", string(bodyByte))
	w.Write(bodyByte)
}

func Start() error {
	listenPort := Conf.ListenPort
	listenIp := Conf.ListenIp
	if listenPort <= 0 || listenPort > 65535 {
		log.Println("invalid listen port:", listenPort)
		return errors.New("invalid listen port")
	}

	http.HandleFunc("/reflect", reflect)
	err := http.ListenAndServeTLS(listenIp+":"+strconv.Itoa(listenPort), Conf.Protocol.Https.Certificate, Conf.Protocol.Https.Key, nil)
	return err
}

// ------------------TLS------------------------//

func TLSDataHandle(conn net.Conn) {
	for {
		// 读取request
		ioBuf := bufio.NewReader(conn)
		req, err := http.ReadRequest(ioBuf)
		if err != nil {
			log.Println(err)
			return
		}
		defer req.Body.Close()
		defer conn.Close()
		bodyByte, _ := io.ReadAll(req.Body)
		log.Println("recv: ", string(bodyByte))

		// 构建一个response
		buf := bytes.NewBuffer(nil)
		buf.WriteString("HTTP/1.1 200 OK\r\n")
		buf.WriteString("Content-Length: " + strconv.Itoa(len(bodyByte)) + "\r\n")
		buf.WriteString("\r\n")
		buf.Write(bodyByte)

		// 发送response
		buf.WriteTo(conn)
	}
}

func TLSStart() error {
	listenPort := Conf.ListenPort
	listenIp := Conf.ListenIp
	if listenPort <= 0 || listenPort > 65535 {
		log.Println("invalid listen port:", listenPort)
		return errors.New("invalid listen port")
	}

	cert, err := tls.LoadX509KeyPair(Conf.Protocol.Https.Certificate, Conf.Protocol.Https.Key)
	if err != nil {
		log.Println("fail to laod x509 key pair", err)
	}

	config := &tls.Config{Certificates: []tls.Certificate{cert}}
	listener, _ := tls.Listen("tcp", listenIp+":"+strconv.Itoa(listenPort), config)
	for {
		conn, _ := listener.Accept()
		go TLSDataHandle(conn)
	}
}
