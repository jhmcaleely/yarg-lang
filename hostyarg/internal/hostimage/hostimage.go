package hostimage

import "path/filepath"

func CmdBuildLib(libDir, outputFile string) error {
	libDir = filepath.Clean(libDir)
	outputFile = filepath.Clean(outputFile)
	return nil
}
