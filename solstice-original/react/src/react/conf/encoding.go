package conf

import (
	"encoding/json"
	"io"
	"log"
	"os"
)

func Marshal() ([]byte, error) {
	return json.MarshalIndent(&Conf, "", "    ")
}

func String() string {
	buf, e := Marshal()
	if e != nil {
		panic(e)
	}
	return string(buf)
}

func SaveFile(path string) error {
	buf, e := Marshal()
	if e != nil {
		return e
	}

	fout, e := os.Create(path)
	if e != nil {
		return e
	}

	_, e = fout.Write(buf)
	if e != nil {
		return e
	}

	return fout.Close()
}

func ReadFrom(r io.Reader) error {
	return json.NewDecoder(r).Decode(&Conf)
}

func LoadFile(path string) error {
	fin, e := os.Open(path)
	if e != nil {
		return e
	}

	e = ReadFrom(fin)
	if e != nil {
		return e
	}

	return fin.Close()
}

const DefaultPath = "react.conf"

func Save() error { return SaveFile(DefaultPath) }

func Load() {
	e := LoadFile(DefaultPath)
	if os.IsNotExist(e) {
		return
	}
	if e != nil {
		log.Fatal(e)
	}
}
