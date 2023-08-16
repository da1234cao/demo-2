package server

import (
	"encoding/json"
	"log"
	"os"
)

type configure struct {
	ListenIp   string `json:"listenIp"`
	ListenPort int    `json:"listenPort"`
	Protocol   struct {
		Https struct {
			Noused      bool   `json:"notUsed"`
			Key         string `json:"key"`
			Certificate string `json:"Certificate"`
		}
	}
}

var Conf configure

func LoadConfig(confPath string) {
	file, err := os.OpenFile(confPath, os.O_RDWR, 0755)
	if err != nil {
		// log.WithError(err).Error("fail to open config file")
		log.Println("fail to open config file:", err)
		return
	}
	defer file.Close()

	err = json.NewDecoder(file).Decode(&Conf)
	if err != nil {
		log.Println("fail to decode config file", err)
	}
	log.Println("read config file success:", Conf)
}
