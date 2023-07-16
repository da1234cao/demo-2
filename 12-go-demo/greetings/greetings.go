package greetings // 声明一个 greetings 包以收集相关函数

import (
	"errors"
	"fmt"
)

func Hello(name string) (string, error) {
	if name == "" {
		return "", errors.New("empty name")
	}
	message := fmt.Sprintf("hi, %v. Welcome", name)
	return message, nil
}
