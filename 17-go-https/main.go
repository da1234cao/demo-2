package main

import (
	"flag"
	"fmt"
	"go-https-demo/certificate"
	"go-https-demo/client"
	"go-https-demo/server"
	"log"
	"os"
)

type SecondFlag struct {
	Name       string
	FlagSet    *flag.FlagSet
	CmdComment string
}

var (
	action        int
	confPath      string
	Subcommands   map[string]*SecondFlag
	genCertOutDir string
)

const (
	startServer = iota + 1
	startClient
	genCert
)

func init() {
	// 二级命令，ref:https://www.cnblogs.com/yahuian/p/go-flag.html
	servFlag := &SecondFlag{
		Name:       "server",
		FlagSet:    flag.NewFlagSet("server", flag.ExitOnError),
		CmdComment: "server command",
	}
	servFlag.FlagSet.StringVar(&confPath, "config", "", "server config file path")

	clientFlag := &SecondFlag{
		Name:       "client",
		FlagSet:    flag.NewFlagSet("client", flag.ExitOnError),
		CmdComment: "client command",
	}
	clientFlag.FlagSet.StringVar(&confPath, "config", "", "client config file path")

	genCertFlag := &SecondFlag{
		Name:       "genCert",
		FlagSet:    flag.NewFlagSet("genCert", flag.ExitOnError),
		CmdComment: "generate certificate",
	}
	genCertFlag.FlagSet.StringVar(&genCertOutDir, "outdir", "./", "certificate output path")

	Subcommands = map[string]*SecondFlag{
		servFlag.Name:    servFlag,
		clientFlag.Name:  clientFlag,
		genCertFlag.Name: genCertFlag,
	}

	useage := func() { // 整个命令行的帮助信息
		fmt.Printf("Usage: https COMMAND\n\n")
		for k, v := range Subcommands {
			fmt.Printf("%s %s\n", k, v.CmdComment)
			v.FlagSet.PrintDefaults()
			fmt.Println()
		}
	}

	// 即没有输入子命令
	// 第二个参数必须是我们支持的子命令
	if len(os.Args) < 2 || Subcommands[os.Args[1]] == nil {
		useage()
		os.Exit(0)
	}
}

func main() {
	if os.Args[1] == Subcommands["server"].Name {
		action = startServer
	} else if os.Args[1] == Subcommands["client"].Name {
		action = startClient
	} else if os.Args[1] == Subcommands["genCert"].Name {
		action = genCert
	}

	cmd := Subcommands[os.Args[1]]
	cmd.FlagSet.Parse(os.Args[2:]) // 注意这里是 cmd.Parse 不是 flag.Parse，且值是 Args[2:]

	if action == startServer {
		server.LoadConfig(confPath)
		// https服务端
		// server.Start()
		// tls服务端
		server.TLSStart()
	} else if action == startClient {
		client.LoadConfig(confPath)
		// https客户端
		// cli := client.New()
		// client.PrintResponse(client.DoRequest(cli, []byte("hello world")))
		// client.PrintResponse(client.DoRequest(cli, []byte("world hello")))
		for i := 0; i < 2; i++ {
			conn, err := client.NewTlsConn()
			if err != nil {
				log.Println(err)
				return
			}
			client.SendRequest(conn, []byte("hello world"))
		}
	} else if action == genCert {
		certificate.Gencertificate(genCertOutDir)
	}
}
