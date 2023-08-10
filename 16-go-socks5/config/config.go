package config

import (
	"encoding/json"
	"os"

	log "github.com/sirupsen/logrus"
)

type configure struct {
	ListenIp   string `json:"ListenIp"`
	ListenPort int    `json:"ListenPort"`
}

var Conf configure

func LoadConfig(confPath string) {
	file, err := os.OpenFile(confPath, os.O_RDWR, 0755)
	if err != nil {
		// log.WithError(err).Error("fail to open config file")
		log.Error("fail to open config file:", err)
		return
	}
	defer file.Close()

	err = json.NewDecoder(file).Decode(&Conf)
	if err != nil {
		log.WithError(err).Error("fail to decode config file")
	}
	log.Debug(Conf)
}
