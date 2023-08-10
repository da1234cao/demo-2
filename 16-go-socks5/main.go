package main

import (
	"go-socks5-demo/config"
	"go-socks5-demo/socks5"

	nested "github.com/antonfisher/nested-logrus-formatter"
	log "github.com/sirupsen/logrus"
	"gopkg.in/natefinch/lumberjack.v2"
)

func init() {
	logger := &lumberjack.Logger{
		Filename: "logrus.log",
		// 单位是 MB
		MaxSize: 500,
		// 最大过期日志保留的个数
		MaxBackups: 3,
		// 保留过期文件的最大时间间隔,单位是天
		MaxAge: 28, //days
		// 是否需要压缩滚动日志, 使用的 gzip 压缩
		Compress: true, // disabled by default
	}
	log.SetOutput(logger)

	log.SetFormatter(&nested.Formatter{
		NoColors:      true,
		HideKeys:      true,
		ShowFullLevel: true,
	})
	log.SetReportCaller(true)
	log.SetLevel(log.DebugLevel)
}

func main() {
	log.Debug("welcome go socks demo")
	config.LoadConfig("./config.json")
	socks5.Start()
}
