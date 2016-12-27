package replay

import (
	"bytes"
	"fmt"
	"io"
	"strings"
)

type Header struct {
	Name      string
	Memo      string
	Nhost     int
	LinkBw    uint64
	PackBw    uint64
	ChanNames []string
}

func (self *Header) PrintTo(w io.Writer) error {
	buf := new(bytes.Buffer)
	if self.Name != "" {
		fmt.Fprintf(buf, "#%s\n", self.Name)
	}
	if self.Memo != "" {
		fmt.Fprintf(buf, "// %s\n", self.Memo)
	}
	fmt.Fprintf(buf, "n=%d\n", self.Nhost)
	fmt.Fprintf(buf, "linkbw=%d\n", self.LinkBw)
	fmt.Fprintf(buf, "packbw=%d\n", self.PackBw)
	fmt.Fprintf(buf, "chans=%s\n", strings.Join(self.ChanNames, ","))

	_, e := buf.WriteTo(w)
	return e
}
