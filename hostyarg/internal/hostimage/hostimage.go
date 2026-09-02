package hostimage

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"io/fs"
	"math"
	"os"
	"path/filepath"
)

type LibraryImageHeader struct {
	Version        uint32
	Length         uint32
	NodeZeroOffset uint32
}

type LibraryNodeEntry struct {
	Length    uint32
	Alignment uint32
}

type LibraryWriter interface {
	io.WriteSeeker
	io.WriterAt
}

type LibraryDirEntry struct {
	FileNode uint16
	NameNode uint16
}

const version uint32 = 1

var endianness = binary.LittleEndian

func binaryWriteAt(w io.WriterAt, order binary.ByteOrder, data interface{}, offset uint32) (err error) {
	buf := new(bytes.Buffer)
	err = binary.Write(buf, order, data)
	if err != nil {
		return err
	}
	_, err = w.WriteAt(buf.Bytes(), int64(offset))
	return err
}

func writeLibraryImageHeader(w io.WriterAt, length uint32) (err error) {

	header := LibraryImageHeader{
		Version:        version,
		Length:         length,
		NodeZeroOffset: uint32(binary.Size(LibraryImageHeader{})),
	}

	err = binaryWriteAt(w, endianness, header, 0)
	if err != nil {
		return err
	}
	return nil
}

func nodePadding(startLen uint32, alignment uint) uint32 {
	padding := uint32(alignment) - (startLen % uint32(alignment))
	if padding == uint32(alignment) {
		return 0
	}
	return padding
}

func writeLibraryPadding(w io.WriterAt, startLen uint32, alignment uint) (err error) {
	padding := nodePadding(startLen, alignment)
	if padding != 0 {
		_, err = w.WriteAt(make([]byte, padding), int64(startLen))
		return err
	}
	return nil
}

func writeStringNode(w LibraryWriter, s string, alignment uint) (err error) {
	c_string := append([]byte(s), 0)

	return writeLibraryNode(w, c_string, alignment)
}

func writeLibraryNode(w LibraryWriter, node []byte, alignment uint) (err error) {
	startLen64, err := w.Seek(0, io.SeekEnd)
	if err != nil {
		return err
	}
	startLen := uint32(startLen64)
	err = writeLibraryPadding(w, startLen, alignment)
	if err != nil {
		return err
	}
	paddedStartLen64, err := w.Seek(0, io.SeekEnd)
	if err != nil {
		return err
	}
	paddedStartLen := uint32(paddedStartLen64)
	dataLength := uint32(len(node))
	_, err = w.Write(node)
	err = writeLibraryImageHeader(w, paddedStartLen+dataLength)
	return err
}

func writeLibraryIndex(w LibraryWriter, lengths []LibraryNodeEntry) (err error) {

	indexNode := new(bytes.Buffer)
	indexOffset := uint32(12)
	indexLength := len(lengths)*8 + 8
	indexOffset += nodePadding(indexOffset, 4)

	binary.Write(indexNode, endianness, indexOffset)
	binary.Write(indexNode, endianness, uint32(indexLength))

	var offset uint32 = indexOffset + uint32(indexLength)
	for _, length := range lengths {
		err = binary.Write(indexNode, endianness, offset)
		if err != nil {
			return err
		}
		offset += length.Length
		offset += nodePadding(offset, uint(length.Alignment))
		err = binary.Write(indexNode, endianness, length.Length)
		if err != nil {
			return err
		}
	}
	return writeLibraryNode(w, indexNode.Bytes(), 4)
}

func writeDirectory(w LibraryWriter, directoryEntries []LibraryDirEntry) (err error) {
	dirNode := new(bytes.Buffer)
	for _, entry := range directoryEntries {
		err = binary.Write(dirNode, endianness, entry.FileNode)
		if err != nil {
			return err
		}
		err = binary.Write(dirNode, endianness, entry.NameNode)
		if err != nil {
			return err
		}
	}
	return writeLibraryNode(w, dirNode.Bytes(), 2)
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

	lengths := make([]LibraryNodeEntry, 0)
	directoryEntries := make([]LibraryDirEntry, 0)
	nodeCursor := uint16(2)
	for _, entry := range entries {
		info, err := entry.Info()
		if err != nil {
			return err
		}
		if info.IsDir() {
			continue
		}
		if info.Size() > math.MaxUint32 {
			return fmt.Errorf("file %s is too large", entry.Name())
		}
		lengths = append(lengths, LibraryNodeEntry{Length: uint32(info.Size()), Alignment: 1})
		lengths = append(lengths, LibraryNodeEntry{Length: uint32(len(entry.Name()) + 1), Alignment: 1})
		directoryEntries = append(directoryEntries, LibraryDirEntry{FileNode: nodeCursor, NameNode: nodeCursor + 1})
		nodeCursor += 2
	}

	err = writeLibraryImageHeader(libraryimage, uint32(binary.Size(LibraryImageHeader{})))
	if err != nil {
		return err
	}

	nodeLength := make([]LibraryNodeEntry, 0)
	nodeLength = append(nodeLength, LibraryNodeEntry{Length: uint32(len(directoryEntries)) * uint32(binary.Size(LibraryDirEntry{})), Alignment: 2})
	nodeLength = append(nodeLength, lengths...)

	err = writeLibraryIndex(libraryimage, nodeLength)
	if err != nil {
		return err
	}

	err = writeDirectory(libraryimage, directoryEntries)
	if err != nil {
		return err
	}

	lengthIndex := 0
	for _, entry := range entries {
		info, err := entry.Info()
		if err != nil {
			return err
		}
		if info.IsDir() {
			continue
		}
		if info.Size() > math.MaxUint32 {
			return fmt.Errorf("file %s is too large", entry.Name())
		}

		data, err := fs.ReadFile(filesystem, entry.Name())
		if err != nil {
			return err
		}
		err = writeLibraryNode(libraryimage, data, uint(lengths[lengthIndex].Alignment))
		if err != nil {
			return err
		}
		lengthIndex++
		err = writeStringNode(libraryimage, entry.Name(), uint(lengths[lengthIndex].Alignment))
		if err != nil {
			return err
		}
		lengthIndex++
	}
	return nil
}
