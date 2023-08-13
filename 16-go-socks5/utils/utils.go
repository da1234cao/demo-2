package utils

import "sync"

var SPool = sync.Pool{
	New: func() interface{} {
		return make([]byte, 512)
	},
}
