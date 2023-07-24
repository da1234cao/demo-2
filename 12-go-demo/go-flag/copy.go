package main

import (
	"flag"
	"fmt"

	cp "github.com/otiai10/copy"
)

var (
	srcPath string
	dstPath string
)

func init() {
	flag.StringVar(&srcPath, "src", "", "source path")
	flag.StringVar(&dstPath, "dst", "", "dest path")
}

func main() {
	flag.Parse()
	err := cp.Copy(srcPath, dstPath)
	if err != nil {
		fmt.Println(err)
	}
}
