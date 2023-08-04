package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
)

func main() {
	conn, err := net.Dial("tcp", "127.0.0.1:10000")
	if err != nil {
		fmt.Println("Error dial", err)
		return
	}
	defer conn.Close()

	inputReader := bufio.NewReader(os.Stdin)
	for {
		input, isPrefix, err := inputReader.ReadLine()
		if isPrefix || err != nil {
			fmt.Println("The entered single line content is too long or there is an error", err)
			continue
		}
		_, err = conn.Write(input)
		if err != nil {
			fmt.Println("Error in conn write", err)
			continue
		}

		buf := [512]byte{}
		n, rerr := conn.Read(buf[:])
		if rerr != nil {
			fmt.Println("Error in conn read", err)
			continue
		}
		fmt.Println(string(buf[:n]))
	}
}
