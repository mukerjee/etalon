package simlog

import (
	"encoding/json"
	"io"
	"os"
)

type MatReader struct {
	fin *os.File
	dec *json.Decoder
}

func NewMatReader(path string) *MatReader {
	fin, e := os.Open(path)
	ne(e)

	ret := new(MatReader)
	ret.fin = fin
	ret.dec = json.NewDecoder(ret.fin)
	return ret
}

func (self *MatReader) Read(e *MatEntry) bool {
	ret := self.dec.Decode(e)
	if ret == io.EOF {
		ne(self.fin.Close())
		return false
	}
	ne(ret)
	return true
}
