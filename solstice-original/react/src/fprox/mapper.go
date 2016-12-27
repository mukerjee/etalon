package fprox

type Mapper interface {
	MapRead(path string) (string, error)
	MapWrite(path string) (string, error)
}

var DirectMapper = new(directMapper)
var _ Mapper = new(directMapper)

type directMapper struct{}

func (self *directMapper) MapRead(path string) (string, error) {
	return path, nil
}
func (self *directMapper) MapWrite(path string) (string, error) {
	return path, nil
}
