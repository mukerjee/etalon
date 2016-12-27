package demand

import (
	"fmt"
	"io"
)

type header struct {
	Name  string
	Memo  string
	Nhost int
}

func (self *header) PrintTo(w io.Writer) error {
	var e error
	if self.Name != "" {
		_, e = fmt.Fprintf(w, "# %s\n", self.Name)
		if e != nil {
			return e
		}
	}

	if self.Memo != "" {
		_, e = fmt.Fprintf(w, "// %s\n", self.Memo)
		if e != nil {
			return e
		}
	}

	_, e = fmt.Fprintf(w, "n=%d\n", self.Nhost)
	if e != nil {
		return e
	}

	return nil
}
