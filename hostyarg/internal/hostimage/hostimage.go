package hostimage

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"io/fs"
	"os"
	"path/filepath"
)

type LibraryImageEntry struct {
	Name   string
	Offset uint16
	Length uint16
}

func writeLibraryImage(outputFile io.Writer, files map[string][]byte) error {

	imageHeader := make(map[string]LibraryImageEntry)

	for filename, data := range files {
		if len(filename) > 255 {
			return fmt.Errorf("file name '%s' is too long", filename)
		}
		if len(data) > 65535 {
			return fmt.Errorf("file '%s' is too large", filename)
		}
		imageentry := LibraryImageEntry{
			Name:   filename,
			Offset: 0, // Placeholder, will be updated later
			Length: uint16(len(data)),
		}
		imageHeader[filename] = imageentry
	}

	offsets := make(map[string]int)
	currentOffset := 0
	for name, data := range files {
		offsets[name] = currentOffset
		currentOffset += len(data)
	}

	err := binary.Write(outputFile, binary.LittleEndian, uint16(len(files)))
	if err != nil {
		return err
	}
	if flusher, ok := outputFile.(interface{ Flush() error }); ok {
		flusher.Flush()
	}

	b := bytes.Buffer{}
	toc := bufio.NewWriter(&b)

	for name, data := range files {
		err := binary.Write(toc, binary.LittleEndian, uint8(len(name)))
		if err != nil {
			return err
		}
		_, err = toc.Write([]byte(name))
		if err != nil {
			return err
		}
		err = binary.Write(toc, binary.LittleEndian, uint16(len(data)))
		if err != nil {
			return err
		}
		err = binary.Write(toc, binary.LittleEndian, uint16(offsets[name]))
		if err != nil {
			return err
		}
	}
	toc.Flush()

	_, err = outputFile.Write(b.Bytes())
	if err != nil {
		return err
	}

	for _, data := range files {
		_, err := outputFile.Write(data)
		if err != nil {
			return err
		}
	}

	return nil
}

func CmdBuildLib(libDir, outputFile string) error {
	libDir = filepath.Clean(libDir)
	outputFile = filepath.Clean(outputFile)

	libraryimage, err := os.Create(outputFile)
	if err != nil {
		return err
	}
	defer libraryimage.Close()

	filesystem := os.DirFS(libDir)

	entries, err := fs.ReadDir(filesystem, ".")
	if err != nil {
		return err
	}

	files := make(map[string][]byte)

	for _, entry := range entries {
		info, err := entry.Info()
		if err != nil {
			return err
		}
		if info.IsDir() {
			continue
		}

		data, err := fs.ReadFile(filesystem, entry.Name())
		if err != nil {
			return err
		}
		files[entry.Name()] = data
	}

	return writeLibraryImage(libraryimage, files)

}
