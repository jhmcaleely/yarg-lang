package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"log"
	"os"
)

func ensure_mount(cfg LittleFsConfig) LittleFs {
	var lfs LittleFs

	lfs, err := cfg.Mount()
	if err != nil {
		cfg.Format()
		lfs, _ = cfg.Mount()
	}
	return lfs
}

func update_boot_count(lfs LittleFs) {

	file := newLfsFile(lfs)
	file.Open("boot_count")
	defer file.Close()

	var boot_count uint32
	binary.Read(file, binary.LittleEndian, &boot_count)

	boot_count += 1
	file.Rewind()

	binary.Write(file, binary.LittleEndian, boot_count)

	fmt.Printf("boot count: %d\n", boot_count)
}

func add_file(lfs LittleFs, fileToAdd string) {

	r, err := os.Open(fileToAdd)
	if err != nil {
		log.Fatal("nothing to open")
	}
	defer r.Close()
	data, err := io.ReadAll(r)
	if err != nil {
		log.Fatal("nothing read")
	}

	file, _ := lfs.OpenFile(fileToAdd)
	defer file.Close()

	file.Write(data)
}

func list_files(fs LittleFs, dirEntry string) {

	dir, err := fs.OpenDir(dirEntry)
	if err != nil {
		log.Fatal(err.Error())
	}
	defer dir.Close()

	for more, info, err := dir.Read(); more; more, info, err = dir.Read() {
		if err != nil {
			log.Fatal(err)
		}
		switch info.Type {
		case EntryTypeDir:
			fmt.Printf("'%v' (dir)\n", info.Name)
		case EntryTypeReg:
			fmt.Printf("'%v' (%v)\n", info.Name, info.Size)
		default:
			log.Fatal("unexpected entry type.")
		}
	}

}

func (bd BlockDevice) readFromUF2File(filename string) (e error) {
	f, e := os.Open(filename)
	if e == nil {
		defer f.Close()
		bd.ReadFromUF2(f)
	}
	return e
}

func (bd BlockDevice) writeToUF2File(filename string) error {
	f, err := os.OpenFile(filename, os.O_RDWR|os.O_CREATE, 0666)
	if err != nil {
		log.Fatal(err)
	}
	defer f.Close()

	bd.WriteAsUF2(f)

	return nil
}

func cmdBootCountDemo(fs BdFS, fsFilename string) {
	fs.flash_fs.device.readFromUF2File(fsFilename)

	lfs := ensure_mount(fs.cfg)
	defer lfs.Close()

	update_boot_count(lfs)

	fs.flash_fs.device.writeToUF2File(fsFilename)
}

func formatCmd(fs BdFS, fsFilename string) {
	fs.flash_fs.device.readFromUF2File(fsFilename)

	fs.cfg.Format()

	fs.flash_fs.device.writeToUF2File(fsFilename)
}

func cmdAddFile(fs BdFS, fsFilename, fileToAdd string) {
	fs.flash_fs.device.readFromUF2File(fsFilename)

	lfs, _ := fs.cfg.Mount()
	defer lfs.Close()

	add_file(lfs, fileToAdd)

	fs.flash_fs.device.writeToUF2File(fsFilename)
}

func cmdLs(fs BdFS, fsFilename, dirEntry string) {
	fs.flash_fs.device.readFromUF2File(fsFilename)

	lfs, _ := fs.cfg.Mount()
	defer lfs.Close()

	list_files(lfs, dirEntry)
}

func main() {

	log.SetOutput(io.Discard)

	bootCountDemoCmd := flag.NewFlagSet("bootcount", flag.ExitOnError)
	bootCountFS := bootCountDemoCmd.String("fs", "test.uf2", "mount and increment boot_count on fs")

	formatFSCmd := flag.NewFlagSet("format", flag.ExitOnError)
	formatFSFS := formatFSCmd.String("fs", "test.uf2", "format fs on this image")

	addFileCmd := flag.NewFlagSet("addfile", flag.ExitOnError)
	addFileFS := addFileCmd.String("fs", "test.uf2", "add file to this filesystem")
	addFileName := addFileCmd.String("add", "", "filename to add")

	lsDirCmd := flag.NewFlagSet("ls", flag.ExitOnError)
	lsDirFS := lsDirCmd.String("fs", "test.uf2", "filesystem to mount")
	lsDirEntry := lsDirCmd.String("dir", "/", "directory to ls")

	if len(os.Args) < 2 {
		log.Fatal("expected command")
	}

	device := newBlockDevice()
	defer device.Close()

	fs := newBdFS(device, FLASHFS_BASE_ADDR, FLASHFS_BLOCK_COUNT)
	defer fs.Close()

	switch os.Args[1] {
	case "bootcount":
		bootCountDemoCmd.Parse(os.Args[2:])
		cmdBootCountDemo(*fs, *bootCountFS)
	case "format":
		formatFSCmd.Parse(os.Args[2:])
		formatCmd(*fs, *formatFSFS)
	case "addfile":
		addFileCmd.Parse(os.Args[2:])
		if *addFileName == "" {
			log.Fatal("expect filename to add")
		}
		cmdAddFile(*fs, *addFileFS, *addFileName)
	case "ls":
		lsDirCmd.Parse((os.Args[2:]))
		cmdLs(*fs, *lsDirFS, *lsDirEntry)
	default:
		log.Fatal("unknown command")
	}
}
