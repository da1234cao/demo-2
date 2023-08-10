package socks5

import (
	"bufio"
	"encoding/binary"
	"errors"
	"fmt"
	"go-socks5-demo/config"
	"go-socks5-demo/utils"
	"io"
	"net"
	"strconv"

	log "github.com/sirupsen/logrus"
)

const SOCKS5VERSION uint8 = 5

const (
	MethodNoAuth uint8 = iota
	MethodGSSAPI
	MethodUserPass
	MethodNoAcceptable uint8 = 0xFF
)

const (
	RequestConnect uint8 = iota + 1
	RequestBind
	RequestUDP
)

const (
	RequestAtypIPV4       uint8 = iota
	RequestAtypDomainname uint8 = 3
	RequestAtypIPV6       uint8 = 4
)

const (
	Succeeded uint8 = iota
	Failure
	Allowed
	NetUnreachable
	HostUnreachable
	ConnRefused
	TTLExpired
	CmdUnsupported
	AddrUnsupported
)

type Proxy struct {
	Inbound struct {
		reader *bufio.Reader
		writer net.Conn
	}
	Request struct {
		atyp uint8
		addr string
	}
	OutBound struct {
		reader *bufio.Reader
		writer net.Conn
	}
}

func Start() error {
	// 读取配置文件中的监听地址和端口
	log.Debug("socks5 server start")
	listenPort := config.Conf.ListenPort
	listenIp := config.Conf.ListenIp
	if listenPort <= 0 || listenPort > 65535 {
		log.Error("invalid listen port:", listenPort)
		return errors.New("invalid listen port")
	}

	//创建监听
	addr, _ := net.ResolveTCPAddr("tcp", listenIp+":"+strconv.Itoa(listenPort))
	listener, err := net.ListenTCP("tcp", addr)
	if err != nil {
		log.Error("fail in listen port:", listenPort, err)
		return errors.New("fail in listen port")
	}

	// 建立连接
	for {
		conn, _ := listener.Accept()
		go socks5Handle(conn)
	}
}

func socks5Handle(conn net.Conn) {
	proxy := &Proxy{}
	proxy.Inbound.reader = bufio.NewReader(conn)
	proxy.Inbound.writer = conn

	err := handshake(proxy)
	if err != nil {
		log.Warn("fail in handshake", err)
		return
	}
	transport(proxy)
}

func handshake(proxy *Proxy) error {
	err := auth(proxy)
	if err != nil {
		log.Warn(err)
		return err
	}

	err = readRequest(proxy)
	if err != nil {
		log.Warn(err)
		return err
	}

	err = replay(proxy)
	if err != nil {
		log.Warn(err)
		return err
	}
	return err
}

func auth(proxy *Proxy) error {
	/*
		Read
		   +----+----------+----------+
		   |VER | NMETHODS | METHODS  |
		   +----+----------+----------+
		   | 1  |    1     | 1 to 255 |
		   +----+----------+----------+
	*/
	buf := utils.SPool.Get().([]byte)
	defer utils.SPool.Put(buf)

	n, err := io.ReadFull(proxy.Inbound.reader, buf[:2])
	if n != 2 {
		return errors.New("fail to read socks5 request:" + err.Error())
	}

	ver, nmethods := uint8(buf[0]), int(buf[1])
	if ver != SOCKS5VERSION {
		return errors.New("only support socks5 version")
	}
	_, err = io.ReadFull(proxy.Inbound.reader, buf[:nmethods])
	if err != nil {
		return errors.New("fail to read methods" + err.Error())
	}
	supportNoAuth := false
	for _, m := range buf[:nmethods] {
		switch m {
		case MethodNoAuth:
			supportNoAuth = true
		}
	}
	if !supportNoAuth {
		return errors.New("no only support no auth")
	}

	/*
		replay
			+----+--------+
			|VER | METHOD |
			+----+--------+
			| 1  |   1    |
			+----+--------+
	*/
	n, err = proxy.Inbound.writer.Write([]byte{0x05, 0x00}) // 无需认证
	if n != 2 {
		return errors.New("fail to wirte socks method " + err.Error())
	}

	return nil
}

func readRequest(proxy *Proxy) error {
	/*
		Read
		   +----+-----+-------+------+----------+----------+
		   |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
		   +----+-----+-------+------+----------+----------+
		   | 1  |  1  | X'00' |  1   | Variable |    2     |
		   +----+-----+-------+------+----------+----------+
	*/
	// buf := utils.SPool.Get().([]byte)
	// defer utils.SPool.Put(buf)
	buf := make([]byte, 128)
	n, err := io.ReadFull(proxy.Inbound.reader, buf[:4])
	if n != 4 {
		return errors.New("fail to read request " + err.Error())
	}
	ver, cmd, _, atyp := uint8(buf[0]), uint8(buf[1]), uint8(buf[2]), uint8(buf[3])
	if ver != SOCKS5VERSION {
		return errors.New("only support socks5 version")
	}
	if cmd != RequestConnect {
		return errors.New("only support connect requests")
	}
	var addr string
	switch atyp {
	case RequestAtypIPV4:
		_, err = io.ReadFull(proxy.Inbound.reader, buf[:4])
		if err != nil {
			return errors.New("fail in read requests ipv4 " + err.Error())
		}
		addr = string(buf[:4])
	case RequestAtypDomainname:
		_, err = io.ReadFull(proxy.Inbound.reader, buf[:1])
		if err != nil {
			return errors.New("fail in read requests domain len" + err.Error())
		}
		domainLen := int(buf[0])
		_, err = io.ReadFull(proxy.Inbound.reader, buf[:domainLen])
		if err != nil {
			return errors.New("fail in read requests domain " + err.Error())
		}
		addr = string(buf[:domainLen])
	case RequestAtypIPV6:
		_, err = io.ReadFull(proxy.Inbound.reader, buf[:16])
		if err != nil {
			return errors.New("fail in read requests ipv4 " + err.Error())
		}
		addr = string(buf[:16])
	}
	_, err = io.ReadFull(proxy.Inbound.reader, buf[:2])
	if err != nil {
		return errors.New("fail in read requests port " + err.Error())
	}
	port := binary.BigEndian.Uint16(buf[:2])
	proxy.Request.atyp = atyp
	proxy.Request.addr = fmt.Sprintf("%s:%d", addr, port)
	log.Debug("request is", proxy.Request)
	return nil
}

func replay(proxy *Proxy) error {
	/*
		write
		   +----+-----+-------+------+----------+----------+
		   |VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
		   +----+-----+-------+------+----------+----------+
		   | 1  |  1  | X'00' |  1   | Variable |    2     |
		   +----+-----+-------+------+----------+----------+
	*/
	conn, err := net.Dial("tcp", proxy.Request.addr)
	if err != nil {
		log.Warn("fail to connect ", proxy.Request.addr)
		_, rerr := proxy.Inbound.writer.Write([]byte{SOCKS5VERSION, HostUnreachable, 0x00, 0x01, 0, 0, 0, 0, 0, 0})
		if rerr != nil {
			return errors.New("fail in replay " + err.Error())
		}
		return errors.New("fail in connect addr " + proxy.Request.addr + err.Error())
	}
	_, err = proxy.Inbound.writer.Write([]byte{SOCKS5VERSION, Succeeded, 0x00, 0x01, 0, 0, 0, 0, 0, 0})
	if err != nil {
		return errors.New("fail in replay " + err.Error())
	}
	proxy.OutBound.reader = bufio.NewReader(conn)
	proxy.OutBound.writer = conn
	return nil
}

func transport(proxy *Proxy) {
	// 语义上是注释的动作;但是iobuf.reader中无法获取rd值
	// io.Copy(proxy.OutBound.writer, proxy.Inbound.reader)
	go io.Copy(proxy.OutBound.writer, proxy.Inbound.writer) // outbound <- inbound
	// io.Copy(proxy.Inbound.writer, proxy.OutBound.reader)
	go io.Copy(proxy.Inbound.writer, proxy.OutBound.writer) // inbound <- outbound
}
