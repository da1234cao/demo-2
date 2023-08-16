package client

import (
	"bufio"
	"bytes"
	"crypto/tls"
	"io"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"strconv"
)

func New() *http.Client {
	cli := &http.Client{
		Transport: &http.Transport{
			TLSClientConfig: &tls.Config{InsecureSkipVerify: Conf.SkipVerify},
		},
	}
	return cli
}

func DoRequest(cli *http.Client, data []byte) (*http.Response, error) {
	req, err := http.NewRequest("POST", Conf.Protocol+"://"+Conf.ServerIp+":"+strconv.Itoa(Conf.ServerPort)+"/reflect", bytes.NewBuffer(data))
	if err != nil {
		log.Println("fail to consstruct request", err)
	}
	return cli.Do(req)
}

func PrintResponse(resp *http.Response, err error) {
	if err == nil {
		if resp.StatusCode == http.StatusOK {
			body, _ := ioutil.ReadAll(resp.Body)
			log.Print(string(body))
		}
		log.Println("http status code:", resp.StatusCode)
	} else {
		log.Print(err)
	}
}

// ------------------TLS------------------------//
func NewTlsConn() (net.Conn, error) {
	config := &tls.Config{InsecureSkipVerify: Conf.SkipVerify}
	return tls.Dial("tcp", Conf.ServerIp+":"+strconv.Itoa(Conf.ServerPort), config)
}

func SendRequest(conn net.Conn, data []byte) {
	// 构造一个请求
	buf := bytes.NewBuffer(nil)
	buf.WriteString("POST /no_thing")
	buf.WriteString(" HTTP/1.1\r\n")
	buf.WriteString("Content-Length: " + strconv.Itoa(len(data)) + "\r\n")
	buf.WriteString("\r\n")
	buf.Write(data)

	// 发送请求
	buf.WriteTo(conn)

	// 读取回复
	ioBuf := bufio.NewReader(conn)
	res, err := http.ReadResponse(ioBuf, nil)
	if err != nil {
		log.Println(err)
		return
	}
	defer res.Body.Close()
	bodyByte, _ := io.ReadAll(res.Body)
	log.Println(string(bodyByte))
}
