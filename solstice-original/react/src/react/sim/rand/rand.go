package react

import (
	"math/rand"
	"time"
)

func seed() int64 {
	return time.Now().UTC().UnixNano()
}

// Seed the global random in math/rand with UnixNano()
func SeedWithTime() {
	rand.Seed(seed())
}
