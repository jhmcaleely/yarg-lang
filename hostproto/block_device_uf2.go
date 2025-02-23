package main

import (
	"encoding/binary"
	"io"
	"log"
)

type Uf2Frame struct {
	MagicStart0 uint32
	MagicStart1 uint32
	Flags       uint32
	TargetAddr  uint32
	PayloadSize uint32
	BlockNo     uint32
	NumBlocks   uint32
	Reserved    uint32
	Data        [476]byte
	MagicEnd    uint32
}

const UF2_MAGIC_START0 uint32 = 0x0A324655
const UF2_MAGIC_START1 uint32 = 0x9E5D5157
const UF2_MAGIC_END uint32 = 0x0AB16F30
const UF2_FLAG_NOFLASH uint32 = 0x00000001
const UF2_FLAG_FILECONTAINER uint32 = 0x00001000
const UF2_FLAG_FAMILY_ID uint32 = 0x00002000
const UF2_FLAG_MD5_CHKSUM uint32 = 0x00004000
const UF2_FLAG_EXTENSION_TAGS uint32 = 0x00008000

const PICO_UF2_FAMILYID uint32 = 0xe48bff56

func (device BlockDevice) ReadFromUF2(input io.Reader) {

	frame := Uf2Frame{}
	for binary.Read(input, binary.LittleEndian, &frame) != io.EOF {

		if frame.MagicStart0 != UF2_MAGIC_START0 {
			log.Fatal("bad start0")
		}
		if frame.MagicStart1 != UF2_MAGIC_START1 {
			log.Fatal("bad start1")
		}
		if frame.MagicEnd != UF2_MAGIC_END {
			log.Fatal("bad end")
		}

		// erase a block before writing any pages to it.
		if device.IsBlockStart(frame.TargetAddr) {
			device.EraseBlock(frame.TargetAddr)
		}

		device.WriteBlock(frame.TargetAddr, frame.Data[0:frame.PayloadSize])
	}
}

func (device BlockDevice) WriteAsUF2(output io.Writer) {
	pageTotal := device.CountPages()
	pageCursor := uint32(0)

	for b := range device.BlockCount() {
		for p := range device.PagePerBlock() {
			if device.PagePresent(b, p) {
				frame := Uf2Frame{
					MagicStart0: UF2_MAGIC_START0,
					MagicStart1: UF2_MAGIC_START1,
					Flags:       UF2_FLAG_FAMILY_ID,
					TargetAddr:  device.TargetAddress(b, p),
					PayloadSize: PICO_PROG_PAGE_SIZE,
					BlockNo:     pageCursor,
					NumBlocks:   pageTotal,
					Reserved:    PICO_UF2_FAMILYID, // documented as FamilyID, Filesize or 0.
					MagicEnd:    UF2_MAGIC_END,
				}

				copy(frame.Data[0:frame.PayloadSize], device.ReadBlock(frame.TargetAddr, frame.PayloadSize))

				log.Printf("uf2page: %08x, %d\n", frame.TargetAddr, frame.PayloadSize)

				binary.Write(output, binary.LittleEndian, &frame)

				pageCursor++
			}
		}
	}
}
