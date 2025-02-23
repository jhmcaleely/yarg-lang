package main

/*
#include "bdfs_lfs_hal.h"
*/
import "C"
import (
	"runtime"
	"unsafe"
)

// Defines one region of flash to use for a filesystem. The size is a multiple of
// the 4096 byte erase size. We calculate it's location working back from the end of the
// flash device, so that code flashed at the start of the device will not collide.
// Pico's have a 2Mb flash device, so we're looking to be less than 2Mb.

const (
	// 128 blocks will reserve a 512K filsystem - 1/4 of the 2Mb device on a Pico
	FLASHFS_BLOCK_COUNT = 128
	FLASHFS_SIZE_BYTES  = PICO_ERASE_PAGE_SIZE * FLASHFS_BLOCK_COUNT

	// A start location counted back from the end of the device.
	FLASHFS_BASE_ADDR uint32 = PICO_FLASH_BASE_ADDR + PICO_FLASH_SIZE_BYTES - FLASHFS_SIZE_BYTES
)

type BdFS struct {
	cfg      LittleFsConfig
	flash_fs FlashFS
	gohandle *FlashFS
	pins     *runtime.Pinner
}

func newBdFS(device BlockDevice, baseAddr uint32, blockCount uint32) *BdFS {

	cfg := BdFS{cfg: *newLittleFsConfig(blockCount), flash_fs: FlashFS{device: device, base_address: baseAddr}, pins: &runtime.Pinner{}}

	cfg.gohandle = &cfg.flash_fs

	cfg.pins.Pin(cfg.gohandle)
	cfg.pins.Pin(cfg.cfg.chandle)
	cfg.pins.Pin(cfg.flash_fs.device.storage)

	C.install_bdfs_cb(cfg.cfg.chandle, C.uintptr_t(uintptr(unsafe.Pointer(cfg.gohandle))))
	cfg.pins.Pin(cfg.cfg.chandle.context)

	return &cfg
}

func (fs BdFS) Close() error {
	fs.pins.Unpin()

	return nil
}
