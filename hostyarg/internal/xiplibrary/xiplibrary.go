package xiplibrary

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"io/fs"
	"log"
	"math"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
)

// note ordering and size are designed with alignment on ARM in mind.
type LibraryImageHeader struct {
	Magic          [4]byte
	Endianness     uint16
	Version        uint16
	Length         uint32
	DirectoryNode  uint16
	NodeZeroOffset uint8
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

var packageMagic = [4]byte{'y', 0x0a, 'r', 'g'}

const endiannessMarker uint16 = 0xff43

const packageVersion uint16 = 0x2601

var endianness = binary.LittleEndian

const nodeZeroAlignment = 4

func readLibraryImageHeader(data []byte) (header LibraryImageHeader, err error) {
	buf := bytes.NewReader(data)
	header = LibraryImageHeader{}
	err = binary.Read(buf, endianness, &header)
	if err != nil {
		return header, err
	}
	if header.Magic != packageMagic {
		return header, fmt.Errorf("invalid magic: %v", header.Magic)
	}
	if header.Endianness != endiannessMarker {
		return header, fmt.Errorf("invalid endianness marker: %v", header.Endianness)
	}
	if header.Version != packageVersion {
		return header, fmt.Errorf("invalid version: %v", header.Version)
	}
	if header.NodeZeroOffset == 0 {
		return header, fmt.Errorf("invalid node zero offset: %v", header.NodeZeroOffset)
	}

	return header, nil
}

func binaryWriteAt(w io.WriterAt, order binary.ByteOrder, data any, offset uint32) (err error) {
	buf := new(bytes.Buffer)
	err = binary.Write(buf, order, data)
	if err != nil {
		return err
	}
	_, err = w.WriteAt(buf.Bytes(), int64(offset))
	return err
}

func writeLibraryImageHeader(w io.WriterAt, length uint32, directoryNode uint16) (err error) {

	headerSize := uint32(binary.Size(LibraryImageHeader{}))
	nodeZeroOffset := nodePadding(headerSize, nodeZeroAlignment) + headerSize
	if nodeZeroOffset > math.MaxUint8 {
		return fmt.Errorf("node zero offset too large: %v", nodeZeroOffset)
	}

	header := LibraryImageHeader{
		Magic:          packageMagic,
		Endianness:     endiannessMarker,
		Version:        packageVersion,
		Length:         length,
		NodeZeroOffset: uint8(nodeZeroOffset),
		DirectoryNode:  directoryNode,
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
	err = writeLibraryImageHeader(w, paddedStartLen+dataLength, 3)
	return err
}

func writeLibraryIndex(w LibraryWriter, lengths []LibraryNodeEntry) (err error) {

	indexNode := new(bytes.Buffer)
	indexOffset := uint32(16)
	indexLength := len(lengths)*8 + 8
	indexOffset += nodePadding(indexOffset, nodeZeroAlignment)

	binary.Write(indexNode, endianness, indexOffset)
	binary.Write(indexNode, endianness, uint32(indexLength))

	var offset uint32 = indexOffset + uint32(indexLength)
	for _, length := range lengths {
		offset += nodePadding(offset, uint(length.Alignment))
		err = binary.Write(indexNode, endianness, offset)
		if err != nil {
			return err
		}
		log.Printf("Offset: %d, Length: %d (alignment: %d, test %d)\n", offset, length.Length, length.Alignment, offset%uint32(length.Alignment))
		offset += length.Length
		err = binary.Write(indexNode, endianness, length.Length)
		if err != nil {
			return err
		}
	}
	return writeLibraryNode(w, indexNode.Bytes(), nodeZeroAlignment)
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

func writeStartupFile(w LibraryWriter, startupFile string, alignment uint) (err error) {
	if startupFile == "" {
		return nil
	}
	data, err := os.ReadFile(startupFile)
	if err != nil {
		return err
	}
	return writeLibraryNode(w, data, alignment)
}

func CmdBuildLib(libDir, outputFile, startupFile string) error {
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

	if startupFile != "" {
		info, err := os.Stat(startupFile)
		if err != nil {
			return err
		}
		if info.Size() > math.MaxUint32 {
			return fmt.Errorf("startup file %s is too large", startupFile)
		}
		lengths = append(lengths, LibraryNodeEntry{Length: uint32(info.Size()), Alignment: 8})
	} else {
		lengths = append(lengths, LibraryNodeEntry{Length: 0, Alignment: 1})
	}

	directoryEntries := make([]LibraryDirEntry, 0)
	nodeCursor := uint16(3)
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
		lengths = append(lengths, LibraryNodeEntry{Length: uint32(info.Size()), Alignment: 8})
		lengths = append(lengths, LibraryNodeEntry{Length: uint32(len(entry.Name()) + 1), Alignment: 1})
		directoryEntries = append(directoryEntries, LibraryDirEntry{FileNode: nodeCursor, NameNode: nodeCursor + 1})
		nodeCursor += 2
	}

	err = writeLibraryImageHeader(libraryimage, uint32(binary.Size(LibraryImageHeader{})), 3)
	if err != nil {
		return err
	}

	nodeLength := make([]LibraryNodeEntry, 0)
	nodeLength = append(nodeLength, lengths[0])
	nodeLength = append(nodeLength, LibraryNodeEntry{Length: uint32(len(directoryEntries)) * uint32(binary.Size(LibraryDirEntry{})), Alignment: 2})
	nodeLength = append(nodeLength, lengths[1:]...)

	err = writeLibraryIndex(libraryimage, nodeLength)
	if err != nil {
		return err
	}

	err = writeStartupFile(libraryimage, startupFile, uint(lengths[0].Alignment))
	if err != nil {
		return err
	}

	err = writeDirectory(libraryimage, directoryEntries)
	if err != nil {
		return err
	}

	lengthIndex := 1
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

func nodeDataBuffer(data []byte, nodeOffset uint32, nodeLength uint32) ([]byte, error) {
	if int(nodeOffset)+int(nodeLength) > len(data) {
		return nil, fmt.Errorf("node offset and length exceed contents length")
	}
	return data[nodeOffset : nodeOffset+nodeLength], nil
}

func nodeData(data []byte, node uint16) (nodeBytes []byte, err error) {
	header, err := readLibraryImageHeader(data)
	if err != nil {
		return nil, err
	}
	if header.Version != packageVersion {
		return nil, fmt.Errorf("unsupported library image version %d", header.Version)
	}

	if node == 0 {
		indexOffset := endianness.Uint32(data[header.NodeZeroOffset : header.NodeZeroOffset+4])
		indexLength := endianness.Uint32(data[header.NodeZeroOffset+4 : header.NodeZeroOffset+8])

		if indexOffset != uint32(header.NodeZeroOffset) {
			return nil, fmt.Errorf("index offset %d does not match header node zero offset %d", indexOffset, header.NodeZeroOffset)
		}

		return nodeDataBuffer(data, indexOffset, indexLength)
	} else {
		index, err := nodeData(data, 0)
		if err != nil {
			return nil, err
		}
		nodeCount, err := nodeCount(data)
		if err != nil {
			return nil, err
		}
		if int(node) >= nodeCount {
			return nil, fmt.Errorf("node %d out of range", node)
		}
		nodeOffset := endianness.Uint32(index[node*8 : node*8+4])
		nodeLength := endianness.Uint32(index[node*8+4 : node*8+8])
		return nodeDataBuffer(data, nodeOffset, nodeLength)
	}
}

func nodeCount(data []byte) (int, error) {
	index, err := nodeData(data, 0)
	if err != nil {
		return 0, err
	}
	return len(index) / 8, nil
}

func directoryNode(data []byte) (uint16, error) {
	header, err := readLibraryImageHeader(data)
	if err != nil {
		return 0, err
	}
	if header.DirectoryNode == 0 {
		log.Print("no directory node present")
	}
	nodes, err := nodeCount(data)
	if err != nil {
		return 0, err
	}
	if int(header.DirectoryNode) >= nodes {
		return 0, fmt.Errorf("directory node %d out of range", header.DirectoryNode)
	}
	return header.DirectoryNode, nil
}

func directories(data []byte) (dirs []LibraryDirEntry, err error) {
	numNodes, err := nodeCount(data)
	if err != nil {
		return nil, err
	}

	dirNode, err := directoryNode(data)
	if err != nil {
		return nil, err
	}
	if dirNode == 0 {
		return nil, nil
	}

	dirData, err := nodeData(data, dirNode)
	if err != nil {
		return nil, err
	}
	numEntries := len(dirData) / 4
	directoryEntries := make([]LibraryDirEntry, numEntries)
	for i := range directoryEntries {
		fileNode := endianness.Uint16(dirData[i*4 : i*4+2])
		nameNode := endianness.Uint16(dirData[i*4+2 : i*4+4])

		if fileNode >= uint16(numNodes) {
			return nil, fmt.Errorf("file node %d out of range", fileNode)
		}
		if nameNode >= uint16(numNodes) {
			return nil, fmt.Errorf("name node %d out of range", nameNode)
		}

		directoryEntries[i].FileNode = fileNode
		directoryEntries[i].NameNode = nameNode

	}
	return directoryEntries, nil
}

func CmdLs(fsFilename string, dirEntry string, long bool) (e error) {
	data, e := os.ReadFile(fsFilename)
	if e != nil {
		return e
	}

	_, e = readLibraryImageHeader(data)
	if e != nil {
		return e
	}

	directoryEntries, err := directories(data)
	if err != nil {
		return err
	}

	log.Printf("Directory Entries: %d\n", len(directoryEntries))

	for i, entry := range directoryEntries {
		fileNode := entry.FileNode
		nameNode := entry.NameNode

		nameData, err := nodeData(data, nameNode)
		if err != nil {
			return err
		}
		name := string(nameData[:len(nameData)-1])
		fileData, err := nodeData(data, fileNode)
		if err != nil {
			return err
		}
		log.Printf("Entry %d: File Node=%d, Name Node=%d, Name=%s (%d bytes)\n", i, fileNode, nameNode, name, len(fileData))

		if long {
			fmt.Printf("\t\t%d\t%s\n", len(fileData), name)
		} else {
			fmt.Printf("%s\n", name)
		}
	}

	return nil
}

type FsInfo struct {
	Version          uint16
	Size             uint32
	NodeCount        uint16
	UsefulLength     uint32
	DirectoryEntries uint16
	IndexedEntries   uint16
}

func readFsInfo(data []byte) (FsInfo, error) {
	header, e := readLibraryImageHeader(data)
	if e != nil {
		return FsInfo{}, e
	}

	var info FsInfo
	info.Version = header.Version
	info.Size = header.Length
	numNodes, e := nodeCount(data)
	if e != nil {
		return FsInfo{}, e
	}
	info.NodeCount = uint16(numNodes)

	nodeZero, e := nodeData(data, 0)
	if e != nil {
		return FsInfo{}, e
	}

	var usefulLength uint32 = 0
	for i := range numNodes {
		nodeOffset := endianness.Uint32(nodeZero[i*8 : i*8+4])
		nodeLength := endianness.Uint32(nodeZero[i*8+4 : i*8+8])

		if int(nodeOffset)+int(nodeLength) > len(data) {
			return FsInfo{}, fmt.Errorf("node offset and length exceed contents length")
		}

		usefulLength += nodeLength
	}

	info.UsefulLength = uint32(usefulLength)

	nodeDir, e := nodeData(data, header.DirectoryNode)
	if e != nil {
		return FsInfo{}, e
	}
	info.DirectoryEntries = uint16(len(nodeDir) / 4)
	info.IndexedEntries = uint16(header.DirectoryNode - 1)

	return info, nil
}

func CmdFsInfo(fsFilename string) (e error) {
	data, e := os.ReadFile(fsFilename)
	if e != nil {
		return e
	}

	info, e := readFsInfo(data)
	if e != nil {
		return e
	}

	fmt.Printf("Library Version: %d\n", info.Version)
	fmt.Printf("Library Size: %d bytes\n", info.Size)
	fmt.Printf("Node Count: %d\n", info.NodeCount)
	fmt.Printf("Node Data: %d bytes\n", info.UsefulLength)
	fmt.Printf("Library Overhead: %d bytes\n", int(info.Size)-int(info.UsefulLength))
	fmt.Printf("Directory Entries: %d\n", info.DirectoryEntries)
	fmt.Printf("Indexed Entries: %d\n", info.IndexedEntries)

	for i := uint16(0); i < info.IndexedEntries; i++ {
		node := i + 1
		data, e := nodeData(data, node)
		if e != nil {
			return e
		}
		fmt.Printf("Indexed Node %d Size: %d bytes\n", node, len(data))
	}

	return nil
}

type Token int

const (
	TokenPath Token = iota
	TokenComment
	TokenNode
	TokenFile
	TokenTextFile
	TokenIndexFile
	TokenBootfile
	TokenNewLine
	TokenIdentifier
	TokenEOF
	TokenError
)

func (t Token) String() string {
	switch t {
	case TokenPath:
		return "TokenPath"
	case TokenComment:
		return "TokenComment"
	case TokenNode:
		return "TokenNode"
	case TokenFile:
		return "TokenFile"
	case TokenTextFile:
		return "TokenTextFile"
	case TokenIndexFile:
		return "TokenIndexFile"
	case TokenBootfile:
		return "TokenBootfile"
	case TokenNewLine:
		return "TokenNewLine"
	case TokenIdentifier:
		return "TokenIdentifier"
	case TokenEOF:
		return "TokenEOF"
	case TokenError:
		return "TokenError"
	default:
		return "Unknown"
	}
}

type TokenInfo struct {
	Type  Token
	Value string
}

func (T TokenInfo) String() string {
	return fmt.Sprintf("Type: %v, Value: %s", T.Type, strconv.Quote(T.Value))
}

type State int

const (
	ReadNext State = iota
	ReadNewLine
	ReadNewLine2
	ReadRuneIdentifier
	AddRuneToIdentifier
	ReadComment
	AddRuneToComment
	ReadString
	AddRuneToString
	ReadEscape
	AddEscapeToString
	DispatchNewLine
	DispatchRune
	DispatchToken
	Error
	LastLine
	End
)

func (s State) String() string {
	switch s {
	case ReadNext:
		return "ReadNext"
	case ReadNewLine:
		return "ReadNewLine"
	case ReadNewLine2:
		return "ReadNewLine2"
	case ReadRuneIdentifier:
		return "ReadRuneIdentifier"
	case AddRuneToIdentifier:
		return "AddRuneToIdentifier"
	case DispatchNewLine:
		return "DispatchNewLine"
	case DispatchRune:
		return "DispatchRune"
	case DispatchToken:
		return "DispatchToken"
	case Error:
		return "Error"
	case LastLine:
		return "LastLine"
	case End:
		return "End"
	default:
		return "Unknown"
	}
}

func isNewLineComponent(r rune) bool {
	switch r {
	case '\u000A', '\u000D', '\u000C', '\u000B', '\u0085', '\u2028', '\u2029':
		return true
	default:
		return false
	}
}

func isWhitespace(r rune) bool {
	switch r {
	case ' ', '\t':
		return true
	default:
		return false
	}
}

func isCommentStart(r rune) bool {
	switch r {
	case '#':
		return true
	default:
		return false
	}
}

func isStringStart(r rune) bool {
	switch r {
	case '"':
		return true
	default:
		return false
	}
}

func isStringEnd(r rune) bool {
	switch r {
	case '"':
		return true
	default:
		return false
	}
}

func isEscape(r rune) bool {
	switch r {
	case '\\':
		return true
	default:
		return false
	}
}

// a tokeniser for utf-8 line-oriented command scripts.
// supports "strings" (with escape sequences) and #comments
func tokenise(rd io.Reader) []TokenInfo {

	scanner := bufio.NewReader(rd)

	cursor := 0
	current := ReadNext
	var r rune
	var lastRuneSize int

	readRune := func(next State, eof State) {
		rn, size, e := scanner.ReadRune()
		r = rn
		switch {
		case e != nil && e == io.EOF:
			current = eof
		case e != nil:
			current = Error
		case r == '\ufffd' && size == 1:
			current = Error
		default:
			cursor += size
			lastRuneSize = size
			current = next
		}
	}

	var token TokenInfo
	tokens := []TokenInfo{}
	line, column := 1, 1

	for {
		switch current {
		case ReadNext:
			readRune(DispatchRune, End)
		case DispatchRune:
			column += 1
			token.Value = string(r)
			switch {
			case isNewLineComponent(r):
				token.Type = TokenNewLine
				current = ReadNewLine
			case isWhitespace(r):
				current = ReadNext
			case isCommentStart(r):
				token.Type = TokenComment
				current = ReadComment
			case isStringStart(r):
				token.Type = TokenIdentifier
				token.Value = ""
				current = ReadString
			default:
				token.Type = TokenIdentifier
				current = ReadRuneIdentifier
			}
		case ReadNewLine:
			readRune(ReadNewLine2, End)
		case ReadNewLine2:
			switch {
			case r == '\r' && token.Value == string('\n'),
				r == '\n' && token.Value == string('\r'):
				token.Value += string(r)
				current = DispatchNewLine
			default:
				scanner.UnreadRune()
				cursor -= lastRuneSize
				current = DispatchNewLine
			}
		case ReadRuneIdentifier:
			readRune(AddRuneToIdentifier, DispatchToken)
		case AddRuneToIdentifier:
			switch {
			case isNewLineComponent(r), isWhitespace(r), isCommentStart(r), isStringStart(r):
				scanner.UnreadRune()
				cursor -= lastRuneSize
				current = DispatchToken
			default:
				column += 1
				token.Value += string(r)
				current = ReadRuneIdentifier
			}
		case ReadComment:
			readRune(AddRuneToComment, DispatchToken)
		case AddRuneToComment:
			switch {
			case isNewLineComponent(r):
				scanner.UnreadRune()
				cursor -= lastRuneSize
				current = DispatchToken
			default:
				column += 1
				token.Value += string(r)
				current = ReadComment
			}
		case ReadString:
			readRune(AddRuneToString, DispatchToken)
		case AddRuneToString:
			switch {
			case isStringEnd(r):
				cursor -= lastRuneSize
				current = DispatchToken
			case isEscape(r):
				column += 1
				current = ReadEscape
			default:
				column += 1
				token.Value += string(r)
				current = ReadString
			}
		case ReadEscape:
			readRune(AddEscapeToString, DispatchToken)
		case AddEscapeToString:
			column += 1
			switch {
			case r == '"':
				token.Value += string(r)
				current = ReadString
			default:
				current = Error
			}
		case DispatchNewLine:
			column = 1
			line += 1
			current = DispatchToken
		case DispatchToken:
			tokens = append(tokens, token)
			token = TokenInfo{}
			current = ReadNext
		case Error:
			token.Type = TokenError
			token.Value = fmt.Sprintf("cursor: %d, line: %d, column: %d", cursor, line, column)
			tokens = append(tokens, token)
			return tokens
		case End:
			tokens = append(tokens, TokenInfo{Type: TokenEOF, Value: ""})
			return tokens
		}
	}
}

func tokeniseFile(filePath string) ([]TokenInfo, error) {
	file, e := os.Open(filePath)
	if e != nil {
		return nil, e
	}
	defer file.Close()

	return tokenise(file), nil
}

func tokeniseString(input string) ([]TokenInfo, error) {
	reader := strings.NewReader(input)

	return tokenise(reader), nil
}

type XIPLibrary struct {
	CommandPath  string
	indexedFiles []Command
	namedFiles   []Command
}

type Command interface {
	Execute(*XIPLibrary) error
}

type FileCommand struct {
	SourcePath string
	TargetPath string
	Length     int64
	Alignment  int
}

func (c *FileCommand) String() string {
	return fmt.Sprintf("file \"%s\" \"%s\" (%d|%d)", c.SourcePath, c.TargetPath, c.Length, c.Alignment)
}

func CanonicalSource(lib *XIPLibrary, sourcePath string) string {
	dir := filepath.Dir(lib.CommandPath)
	target := filepath.Join(dir, sourcePath)
	return filepath.Clean(target)
}

func DefaultAlignment(sourcePath string) int {
	if filepath.Ext(sourcePath) == ".yb" {
		return 8
	}
	return 1
}

func (c *FileCommand) Execute(lib *XIPLibrary) error {
	c.SourcePath = CanonicalSource(lib, c.SourcePath)
	if c.TargetPath == "" {
		c.TargetPath = filepath.Base(c.SourcePath)
	}

	info, err := os.Stat(c.SourcePath)
	if err != nil {
		return err
	}
	c.Length = info.Size()
	c.Alignment = DefaultAlignment(c.SourcePath)

	lib.namedFiles = append(lib.namedFiles, c)

	return nil
}

type TextFileCommand struct {
	FileCommand
}

func (c *TextFileCommand) String() string {
	return fmt.Sprintf("txtfile \"%s\" -> \"%s\" (%d|%d)", c.SourcePath, c.TargetPath, c.Length, c.Alignment)
}

func (c *TextFileCommand) Execute(lib *XIPLibrary) error {
	return c.FileCommand.Execute(lib)
}

type IndexFileCommand struct {
	SourcePath string
	Alignment  int
	Length     int64
	Index      uint16
}

func (c *IndexFileCommand) Execute(lib *XIPLibrary) error {
	c.SourcePath = CanonicalSource(lib, c.SourcePath)
	c.Alignment = DefaultAlignment(c.SourcePath)
	info, err := os.Stat(c.SourcePath)
	if err != nil {
		return err
	}
	c.Length = info.Size()

	lib.indexedFiles = append(lib.indexedFiles, c)
	return nil
}

func (c *IndexFileCommand) String() string {
	return fmt.Sprintf("indexfile \"%s\" %d (%d|%d)", c.SourcePath, c.Index, c.Length, c.Alignment)
}

type ErrorCommand struct {
	Message string
}

func (c *ErrorCommand) Execute(lib *XIPLibrary) error {
	return fmt.Errorf("%s", c.Message)
}

func (c *ErrorCommand) String() string {
	return fmt.Sprintf("error \"%s\"", c.Message)
}

func parseACommand(commandTokens []TokenInfo) Command {
	switch commandTokens[0].Type {
	case TokenIdentifier:
		switch commandTokens[0].Value {
		case "file", "txtfile":
			if len(commandTokens) < 2 {
				// Handle error: not enough arguments for file command
				return &ErrorCommand{Message: "not enough arguments for file command"}
			}
			fileSource := commandTokens[1].Value
			targetPath := ""
			if len(commandTokens) == 3 {
				targetPath = commandTokens[2].Value
			}
			fmt.Printf("file command: source=%s, target=%s\n", fileSource, targetPath)
			if commandTokens[0].Value == "txtfile" {
				return &TextFileCommand{FileCommand: FileCommand{SourcePath: fileSource, TargetPath: targetPath}}
			}
			return &FileCommand{SourcePath: fileSource, TargetPath: targetPath}
		case "bootfile":
			if len(commandTokens) < 2 {
				// Handle error: not enough arguments for bootfile command
				return &ErrorCommand{Message: "not enough arguments for bootfile command"}
			}
			bootSource := commandTokens[1].Value
			fmt.Printf("bootfile command: source=%s\n", bootSource)
			return &IndexFileCommand{SourcePath: bootSource, Index: 1}
		case "indexfile":
			if len(commandTokens) < 3 {
				// Handle error: not enough arguments for indexfile command
				return &ErrorCommand{Message: "not enough arguments for indexfile command"}
			}
			indexSource := commandTokens[1].Value
			indexValue, err := strconv.Atoi(commandTokens[2].Value)
			if err != nil {
				return &ErrorCommand{Message: "invalid index value for indexfile command"}
			}
			if indexValue < 0 || indexValue > 65535 {
				return &ErrorCommand{Message: "index value out of range for indexfile command"}
			}
			fmt.Printf("indexfile command: source=%s, index=%d\n", indexSource, indexValue)
			return &IndexFileCommand{SourcePath: indexSource, Index: uint16(indexValue)}
		default:
			return &ErrorCommand{Message: fmt.Sprintf("unknown command: %s", commandTokens[0].Value)}
		}
	}
	return &ErrorCommand{Message: "invalid command"}
}

func parseCommand(token TokenInfo, tokens []TokenInfo) (Command, int) {
	consumed := 0
	commandTokens := []TokenInfo{}

	for {
		switch token.Type {
		case TokenComment:
			// skip
		case TokenNewLine:
			if len(commandTokens) > 0 {
				return parseACommand(commandTokens), consumed
			}
		case TokenEOF:
			if len(commandTokens) > 0 {
				return parseACommand(commandTokens), consumed - 1
			}
		default:
			commandTokens = append(commandTokens, token)
		}
		consumed++
		if len(tokens) == 0 {
			break
		}
		token = tokens[0]
		tokens = tokens[1:]
	}
	return &ErrorCommand{Message: "unexpected end of input"}, consumed
}

func parse(tokens []TokenInfo) ([]Command, error) {

	commands := []Command{}

	for {
		token := tokens[0]
		tokens = tokens[1:]
		switch {
		case token.Type == TokenComment:
			// Skip comments
			continue
		case token.Type == TokenEOF:
			return commands, nil
		case token.Type == TokenIdentifier:
			command, consumed := parseCommand(token, tokens)
			if command != nil {
				commands = append(commands, command)
			}
			tokens = tokens[consumed:]
		}
	}
}

func writeLibrary(lib *XIPLibrary, TargetPath string) error {
	fmt.Printf("Writing library to %s\n", TargetPath)
	libraryimage, err := os.Create(TargetPath)
	if err != nil {
		return err
	}
	defer libraryimage.Close()

	lengths := make([]LibraryNodeEntry, 0)
	directoryEntries := make([]LibraryDirEntry, 0)
	nodeCursor := uint16(1)
	for _, indexedFile := range lib.indexedFiles {
		c := indexedFile.(*IndexFileCommand)
		lengths = append(lengths, LibraryNodeEntry{Length: uint32(c.Length), Alignment: uint32(c.Alignment)})
		nodeCursor++
	}
	lengths = append(lengths,
		LibraryNodeEntry{Length: uint32(len(lib.namedFiles)) *
			uint32(binary.Size(LibraryDirEntry{})), Alignment: 2})
	nodeCursor++
	for _, namedFile := range lib.namedFiles {
		c := namedFile.(*FileCommand)
		lengths = append(lengths, LibraryNodeEntry{
			Length:    uint32(c.Length),
			Alignment: uint32(c.Alignment),
		})
		lengths = append(lengths, LibraryNodeEntry{
			Length:    uint32(len(c.TargetPath) + 1),
			Alignment: 1,
		})
		directoryEntries = append(directoryEntries, LibraryDirEntry{
			FileNode: nodeCursor,
			NameNode: nodeCursor + 1,
		})
		nodeCursor += 2
	}

	err = writeLibraryImageHeader(libraryimage, uint32(binary.Size(LibraryImageHeader{})), 3)
	if err != nil {
		return err
	}
	err = writeLibraryIndex(libraryimage, lengths)
	if err != nil {
		return err
	}
	sortedIndexedFiles := make([]Command, len(lib.indexedFiles))
	copy(sortedIndexedFiles, lib.indexedFiles)
	sort.Slice(sortedIndexedFiles, func(i, j int) bool {
		iCommand := sortedIndexedFiles[i].(*IndexFileCommand)
		jCommand := sortedIndexedFiles[j].(*IndexFileCommand)
		return iCommand.Index < jCommand.Index
	})
	for _, indexedFile := range sortedIndexedFiles {
		c := indexedFile.(*IndexFileCommand)
		data, err := os.ReadFile(c.SourcePath)
		if err != nil {
			return err
		}
		err = writeLibraryNode(libraryimage, data, uint(c.Alignment))
		if err != nil {
			return err
		}
	}

	err = writeDirectory(libraryimage, directoryEntries)
	if err != nil {
		return err
	}
	for _, file := range lib.namedFiles {
		c := file.(*FileCommand)
		data, err := os.ReadFile(c.SourcePath)
		if err != nil {
			return err
		}

		err = writeLibraryNode(libraryimage, data, uint(c.Alignment))
		if err != nil {
			return err
		}
		err = writeStringNode(libraryimage, c.TargetPath, 1)
		if err != nil {
			return err
		}
	}

	return nil
}

func CmdBuildWithContents(libContents string, outputFile string) (e error) {
	stat, e := os.Stat(libContents)
	if e != nil {
		return e
	}
	if stat.IsDir() {
		return fmt.Errorf("libContents should be a file, not a directory")
	}

	lines, e := tokeniseFile(libContents)
	if e != nil {
		return e
	}

	for _, token := range lines {
		fmt.Println(token)
	}

	commands, e := parse(lines)
	if e != nil {
		return e
	}

	lib := &XIPLibrary{CommandPath: libContents}

	for _, command := range commands {
		command.Execute(lib)
		fmt.Printf("%s\n", command)
	}

	err := writeLibrary(lib, outputFile)
	if err != nil {
		return err
	}
	return nil
}
