.PHONY: all fmt test testv tags doc vet lc

all: build

build:
	@ GOPATH=`pwd` go install -v ./src/...

fmt: 
	@ GOPATH=`pwd` gofmt -s -l -w src

vet: 
	@ GOPATH=`pwd` go vet ./src/...

testv:
	@ GOPATH=`pwd` go test -v ./src/...

test:
	@ GOPATH=`pwd` go test ./src/...

clean:
	@ rm -rf pkg bin

fix:
	@ GOPATH=`pwd` go fix ./src/...

tags:
	@ gotags -R src > tags

doc:
	@ GOPATH=`pwd` godoc -http=:8000

lc:
	@ wc -l `find src -name "*.go"`
