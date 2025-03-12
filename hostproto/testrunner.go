package main

import (
	"bufio"
	"bytes"
	"fmt"
	"io/fs"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"reflect"
	"regexp"
)

type Test struct {
	Expectations   int
	Name           string
	FileName       string
	ExpectedOutput []string
	ExpectedError  []string
}

func cmdRunTests(test string) {

	test = filepath.Clean(test)

	info, err := os.Stat(test)
	if err != nil {
		log.Fatal("nothing to test")
	}

	var grandtotal, grandpass int

	if info.IsDir() {
		fileSystem := os.DirFS(test)

		fs.WalkDir(fileSystem, ".", func(path string, d fs.DirEntry, err error) error {
			if err != nil {
				log.Fatal(err)
			}

			if path == "benchmark" {
				return fs.SkipDir
			}

			if !d.IsDir() {
				target := filepath.Join(test, path)
				total, pass := runTestFile(target)
				grandtotal += total
				grandpass += pass
			}
			return nil
		})
	} else {
		total, pass := runTestFile(test)
		grandtotal += total
		grandpass += pass
	}

	fmt.Printf("Total tests: %v, passed: %v\n", grandtotal, grandpass)
}

func testFriendlyName(testfile string) string {
	test_name := filepath.Base(testfile)
	extension := filepath.Ext(testfile)

	return test_name[:len(test_name)-len(extension)]
}

func runTestFile(testfile string) (total, pass int) {

	var test Test
	test.FileName = testfile
	test.Name = testFriendlyName(test.FileName)

	test.parseTestSource()
	total = test.Expectations

	output, error, _ := runTest(test.FileName)

	if reflect.DeepEqual(test.ExpectedOutput[0:len(output)], output) {
		pass = len(output)
	}

	if len(test.ExpectedError) > 0 &&
		reflect.DeepEqual(test.ExpectedError, error) {
		pass += len(error)
	}

	if total == 0 || pass != total {
		fmt.Printf("test: %v\n", test.Name)
		fmt.Printf("tests supplied: %v\n", total)
		fmt.Printf("tests passed: %v\n", pass)
	}

	return total, pass
}

func (test *Test) parseTestSource() {
	file, err := os.Open(test.FileName)
	if err != nil {
		log.Fatal("could not open test")
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	var lineNo int
	for scanner.Scan() {
		lineNo++
		test.parseLine(lineNo, scanner.Text())
	}
}

func (test *Test) parseLine(lineNo int, line string) {
	r := regexp.MustCompile(`// expect: ?(.*)`)
	match := r.FindStringSubmatch(line)
	if match != nil {
		test.ExpectedOutput = append(test.ExpectedOutput, match[1])
		test.Expectations++
	}

	r = regexp.MustCompile(`// (Error.*)`)
	match = r.FindStringSubmatch(line)
	if match != nil {
		expected := fmt.Sprintf("[line %v] %v", lineNo, match[1])
		test.ExpectedError = append(test.ExpectedError, expected)
		test.Expectations++
	}

	r = regexp.MustCompile(`// expect runtime error: (.+)`)
	match = r.FindStringSubmatch(line)
	if match != nil {
		test.ExpectedError = append(test.ExpectedError, match[1])
		test.Expectations++
		test.ExpectedError = append(test.ExpectedError, fmt.Sprintf("[line %v] in script", lineNo))
		test.Expectations++
	}

	r = regexp.MustCompile(`// \[line (\d+)\] (Error.*)`)
	match = r.FindStringSubmatch(line)
	if match != nil {
		test.ExpectedError = append(test.ExpectedError, fmt.Sprintf("[line %v] %v", match[1], match[2]))
		test.Expectations++
	}

}

func parseOutput(stream bytes.Buffer) (lines []string) {
	scanner := bufio.NewScanner(&stream)
	for scanner.Scan() {
		lines = append(lines, scanner.Text())
	}
	return lines
}

func runTest(name string) ([]string, []string, int) {
	stdout, stderr, exit := runtestexecutable(name)
	results := parseOutput(stdout)
	errors := parseOutput(stderr)
	return results, errors, exit
}

func runtestexecutable(name string) (cstdout, cstderr bytes.Buffer, exitcode int) {
	interpreter := exec.Command("/Users/jhm/Developer/proto-lang/bin/clox", name)
	interpreter.Stdout = &cstdout
	interpreter.Stderr = &cstderr

	err := interpreter.Run()
	if err != nil {
		switch e := err.(type) {
		case *exec.Error:
			fmt.Println("failed executing:", err)
		case *exec.ExitError:
			if e.ExitCode() == 70 {
				return cstdout, cstderr, e.ExitCode()
			} else if e.ExitCode() == 65 {
				return cstdout, cstderr, e.ExitCode()
			} else {
				fmt.Print(cstdout.String())
				fmt.Print(cstderr.String())
				fmt.Println("command exit rc =", e.ExitCode())
				return bytes.Buffer{}, bytes.Buffer{}, e.ExitCode()
			}
		default:
			panic(err)
		}
	} else {
		return cstdout, bytes.Buffer{}, 0
	}
	return bytes.Buffer{}, bytes.Buffer{}, 0
}
