package hostimage

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"io/fs"
	"math"
	"os"
	"path/filepath"
	"sort"
)

type LibraryImageEntry struct {
	Name   string
	Offset uint32
	Length uint16
}

type LibraryImage struct {
	Entries []LibraryImageEntry
	Data    [][]byte
}

func (e LibraryImageEntry) String() string {
	return fmt.Sprintf("Name: %s, Offset: %d, Length: %d", e.Name, e.Offset, e.Length)
}

var endianness = binary.LittleEndian

func (e LibraryImageEntry) writeTo(w io.Writer) (err error) {
	if len(e.Name) > math.MaxUint8 {
		return fmt.Errorf("file name '%s' is too long", e.Name)
	}
	err = binary.Write(w, endianness, uint8(len(e.Name)))
	if err != nil {
		return err
	}
	err = binary.Write(w, endianness, []byte(e.Name))
	if err != nil {
		return err
	}

	err = binary.Write(w, endianness, e.Length)
	if err != nil {
		return err
	}
	err = binary.Write(w, endianness, e.Offset)
	return err
}

func writeLibraryImage(outputFile io.Writer, files map[string][]byte) error {

	libraryImage := LibraryImage{
		Entries: make([]LibraryImageEntry, 0, len(files)),
		Data:    make([][]byte, 0, len(files)),
	}

	keys := make([]string, 0, len(files))
	for k := range files {
		keys = append(keys, k)
	}
	sort.Slice(keys, func(i, j int) bool {
		return keys[i] < keys[j]
	})

	for _, filename := range keys {
		data := files[filename]
		if len(filename) > math.MaxUint8 {
			return fmt.Errorf("file name '%s' is too long", filename)
		}
		if len(data) > math.MaxUint16 {
			return fmt.Errorf("file '%s' is too large", filename)
		}
		imageentry := LibraryImageEntry{
			Name:   filename,
			Offset: 0, // Placeholder, will be updated later
			Length: uint16(len(data)),
		}
		libraryImage.Entries = append(libraryImage.Entries, imageentry)
		libraryImage.Data = append(libraryImage.Data, data)
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

	for index, entry := range libraryImage.Entries {
		entry.Offset = uint32(offsets[entry.Name])
		libraryImage.Entries[index] = entry
	}
	for _, entry := range libraryImage.Entries {
		err := entry.writeTo(toc)
		if err != nil {
			return err
		}
	}
	toc.Flush()

	_, err = outputFile.Write(b.Bytes())
	if err != nil {
		return err
	}

	for _, fileData := range libraryImage.Data {
		_, err = outputFile.Write(fileData)
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
