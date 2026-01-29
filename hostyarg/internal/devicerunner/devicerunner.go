package devicerunner

import (
	"fmt"
	"os/exec"

	"github.com/yarg-lang/yarg-lang/hostyarg/internal/runbinary"
)

const openOCDPath = "/Users/jhm/.pico-sdk/openocd/0.12.0+dev"

func CmdResetDevice() {
	resetCommand := exec.Command(openOCDPath+"/openocd",
		"-s", openOCDPath+"/scripts",
		"-f", "interface/cmsis-dap.cfg",
		"-f", "target/rp2040.cfg",
		"-c", "adapter speed 5000",
		"-c", "init; reset; exit")
	resetCommand.Dir = openOCDPath

	output, errors, code, ok := runbinary.RunCommand(resetCommand)
	if ok {
		fmt.Println("Reset output:", output)
		fmt.Println("Reset errors:", errors)
		fmt.Println("Reset exit code:", code)
	} else {
		fmt.Println("Failed to run OpenOCD")
	}
}
