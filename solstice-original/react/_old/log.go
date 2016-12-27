package react

import "fmt"
import "net"
import "io"
import "os"

var logConn io.Writer = logConnect()
var logNoTimestamp = false

func logConnect() io.Writer {
	ret, err := net.Dial("tcp", "localhost:9000")
	if err == nil {
		// fmt.Println("// log to localhost:9000")
		return ret
	}

	// fmt.Println("// log to stderr")
	return os.Stderr
}

func logln(a ...interface{}) {
	logMsg(fmt.Sprintln(a...))
}

func logf(f string, a ...interface{}) {
	logMsg(fmt.Sprintf(f, a...))
}

func log(a ...interface{}) {
	logMsg(fmt.Sprint(a...))
}

func logMsg(s string) {
	t := Time()
	var err error
	if logNoTimestamp || t == 0 {
		_, err = fmt.Fprint(logConn, s)
	} else {
		_, err = fmt.Fprintf(logConn, "[%d]  %s", t, s)
	}
	if err != nil {
		logConn = os.Stderr

		logln(err)
		// logln("// log to stderr")
		logMsg(s) // redo it again
	}
}
