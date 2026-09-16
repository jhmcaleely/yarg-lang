package xiplibrary

import (
	"os"
	"testing"
)

func DoTestHeaderPacking(t *testing.T) (name string, err error) {
	buffer, err := os.CreateTemp(t.TempDir(), "header")
	if err != nil {
		t.Fatal(err)
	}
	defer buffer.Close()

	name = buffer.Name()

	err = writeLibraryImageHeader(buffer, 30, 0)
	if err != nil {
		t.Fatalf("writeLibraryImageHeader failed: %v", err)
	}

	data := make([]byte, 30)
	buffer.ReadAt(data, 0)

	header, err := readLibraryImageHeader(data)
	if err != nil {
		t.Fatalf("readLibraryImageHeader failed: %v", err)
	}

	if header.Magic != packageMagic {
		t.Fatalf("expected magic %v, got %v", packageMagic, header.Magic)
	}
	if header.Endianness != endiannessMarker {
		t.Fatalf("expected endianness %v, got %v", endiannessMarker, header.Endianness)
	}
	if header.Version != packageVersion {
		t.Fatalf("expected version %v, got %v", packageVersion, header.Version)
	}
	if header.Length != 30 {
		t.Fatalf("expected length 30, got %v", header.Length)
	}
	if header.NodeZeroOffset != 16 {
		t.Fatalf("expected NodeZeroOffset 16, got %v", header.NodeZeroOffset)
	}
	return name, nil
}
func TestHeaderPacking(t *testing.T) {

	name, err := DoTestHeaderPacking(t)
	if err != nil {
		t.Fatalf("DoTestHeaderPacking failed: %v", err)
	}
	err = os.Remove(name)
	if err != nil {
		t.Fatalf("Failed to remove temporary file: %v", err)
	}

}

func TestCmdBuildWithContents(t *testing.T) {
	libContents := "../../testdata/test.yarglib"
	outputFile := "testoutputfile"
	startupFile := "teststartupfile"

	err := CmdBuildWithContents(libContents, outputFile, startupFile)
	if err != nil {
		t.Fatalf("CmdBuildWithContents failed: %v", err)
	}
}

type TokenTestCase struct {
	input    string
	expected []TokenInfo
}

var tokenTestCases = []TokenTestCase{
	{
		input: string([]byte{0xDD, 0xdd}),
		expected: []TokenInfo{
			{Type: TokenError, Value: ""},
		},
	},
	{
		input: "t",
		expected: []TokenInfo{
			{Type: TokenLine, Value: "t"},
			{Type: TokenEOF, Value: ""},
		},
	},
	{
		input: "",
		expected: []TokenInfo{
			{Type: TokenEOF, Value: ""},
		},
	},
	{
		input: "t\r",
		expected: []TokenInfo{
			{Type: TokenLine, Value: "t"},
			{Type: TokenEOF, Value: ""},
		},
	},
	{
		input: "t\n\n",
		expected: []TokenInfo{
			{Type: TokenLine, Value: "t"},
			{Type: TokenNewLine, Value: "\n"},
			{Type: TokenEOF, Value: ""},
		},
	},
	{
		input: "test input string\n\rtest\r\n\u2028test\n",
		expected: []TokenInfo{
			{Type: TokenLine, Value: "test input string"},
			{Type: TokenNewLine, Value: "\n\r"},
			{Type: TokenLine, Value: "test"},
			{Type: TokenNewLine, Value: "\r\n"},
			{Type: TokenNewLine, Value: "\u2028"},
			{Type: TokenLine, Value: "test"},
			{Type: TokenEOF, Value: ""},
		},
	},
	{
		input: "single line without newline",
		expected: []TokenInfo{
			{Type: TokenLine, Value: "single line without newline"},
			{Type: TokenEOF, Value: ""},
		},
	},
	{
		input: "\n\n\r\r\n",
		expected: []TokenInfo{
			{Type: TokenNewLine, Value: "\n"},
			{Type: TokenNewLine, Value: "\n\r"},
			{Type: TokenNewLine, Value: "\r\n"},
			{Type: TokenEOF, Value: ""},
		},
	},
	{
		input: "\u000A\u000C\u000D\u000B\u0085\u2028\u2029",
		expected: []TokenInfo{
			{Type: TokenNewLine, Value: "\n"},
			{Type: TokenNewLine, Value: "\u000C"},
			{Type: TokenNewLine, Value: "\r"},
			{Type: TokenNewLine, Value: "\v"},
			{Type: TokenNewLine, Value: "\u0085"},
			{Type: TokenNewLine, Value: "\u2028"},
			{Type: TokenNewLine, Value: "\u2029"},
			{Type: TokenEOF, Value: ""},
		},
	},
	{
		input: string([]byte{'h', 'e', 'l', 'l', 'o', '\r', '\n', 'w', 'o', 'r', 'l', 'd', 0xa0, 'y', 'a', 'r', 'g'}),
		expected: []TokenInfo{
			{Type: TokenLine, Value: "hello"},
			{Type: TokenNewLine, Value: "\r\n"},
			{Type: TokenError, Value: ""},
		},
	},
}

func TestTokeniseString(t *testing.T) {
	for i, testCase := range tokenTestCases {
		tokens, err := tokeniseString(testCase.input)
		if err != nil {
			t.Errorf("test case %d: tokeniseString failed: %v", i, err)
		}
		if len(tokens) != len(testCase.expected) {
			t.Errorf("test case %d: expected %d tokens, got %d", i, len(testCase.expected), len(tokens))
		}
		for j, token := range tokens {
			if token != testCase.expected[j] {
				t.Errorf("test case %d: expected token %v at index %d, got %v", i, testCase.expected[j], j, token)
			}
		}
	}
}
