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

func runTestFile(test string) (total, pass int) {
	test_name := filepath.Base(test)
	extension := filepath.Ext(test)

	parent_name := filepath.Base(filepath.Dir(test))

	test_friendly_name := test_name[:len(test_name)-len(extension)]

	expectedResults := expectedResults(test)
	total = len(expectedResults)

	output := parseTest(test)

	if reflect.DeepEqual(expectedResults[0:len(output)], output) {
		pass = len(output)
	}

	if total == 0 || pass != total {
		fmt.Printf("test suite: %v:%v\n", parent_name, test_friendly_name)
		fmt.Printf("tests supplied: %v\n", total)
		fmt.Printf("tests passed: %v\n", pass)
	}

	return total, pass
}

func expectedResults(name string) []string {
	tests, err := os.ReadFile(name)
	if err != nil {
		log.Fatal("could not read test source")
	}

	r := regexp.MustCompile("// expect: ?(.*)")

	expectedResults := make([]string, 0, 4)

	match := r.FindAllSubmatch(tests, -1)
	for i := range match {
		expectedResults = append(expectedResults, string(match[i][1]))
	}
	return expectedResults
}

func parseTest(name string) []string {
	stdout := runtest(name)
	scanner := bufio.NewScanner(&stdout)
	results := make([]string, 0)
	for scanner.Scan() {
		results = append(results, scanner.Text())
	}
	return results
}

func runtest(name string) bytes.Buffer {
	interpreter := exec.Command("/Users/jhm/Developer/proto-lang/bin/clox", name)
	var stdout, stderr bytes.Buffer
	interpreter.Stdout = &stdout
	interpreter.Stderr = &stderr

	err := interpreter.Run()
	if err != nil {
		switch e := err.(type) {
		case *exec.Error:
			fmt.Println("failed executing:", err)
		case *exec.ExitError:
			if e.ExitCode() == 70 {
				return stdout
			} else if e.ExitCode() == 65 {
				return stdout
			} else {
				fmt.Print(stdout.String())
				fmt.Print(stderr.String())
				fmt.Println("command exit rc =", e.ExitCode())
			}
		default:
			panic(err)
		}
	} else {
		return stdout
	}
	return bytes.Buffer{}
}
