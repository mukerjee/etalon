package headq

import (
	"log"
)

func noError(e error) {
	if e != nil {
		log.Fatalln(e)
	}
}
