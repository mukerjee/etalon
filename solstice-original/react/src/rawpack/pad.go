package rawpack

import (
	"bytes"
)

func Pad(buf *bytes.Buffer) {
	n := len(buf.Bytes())
	if n < minPackSize {
		buf.Write(make([]byte, minPackSize-n))
	}
}
