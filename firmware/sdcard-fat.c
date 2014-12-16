// Adapted from https://github.com/adafruit/SD

#include <stdio.h>
#include "sdcard.h"
#include "sdcard-fat.h"
#include "sdcard-fat-structs.h"
#include "util.h"

#define SDCARD_FAT_CACHE_FOR_READ 1

typedef union {
  uint8_t  data[512]; // Used to access cached file data blocks.
  uint16_t fat16[256]; // Used to access cached FAT16 entries.
  uint32_t fat32[128]; // Used to access cached FAT32 entries.
  dir_t    dir[16]; // Used to access cached directory entries.
  mbr_t    mbr; // Used to access a cached MasterBoot Record.
  fbs_t    fbs; // Used to access to a cached FAT boot sector.
} buffer_t;

buffer_t _sdcard_fat_buffer;
uint32_t _sdcard_fat_bufferBlockNumber;
uint8_t _sdcard_fat_bufferDirty;
uint32_t _sdcard_fat_bufferMirrorBlock;  // block number for mirror FAT
uint32_t _sdcard_fat_allocSearchStart;   // start cluster for alloc search
uint8_t _sdcard_fat_blocksPerCluster;    // cluster size in blocks
uint32_t _sdcard_fat_blocksPerFat;       // FAT size in blocks
uint32_t _sdcard_fat_clusterCount;       // clusters in one FAT
uint8_t _sdcard_fat_clusterSizeShift;    // shift to convert cluster count to block count
uint32_t _sdcard_fat_dataStartBlock;     // first data block number
uint8_t _sdcard_fat_fatCount;            // number of FATs on volume
uint32_t _sdcard_fat_fatStartBlock;      // start block for first FAT
uint8_t _sdcard_fat_fatType;             // volume type (12, 16, OR 32)
uint16_t _sdcard_fat_rootDirEntryCount;  // number of entries in FAT16 root dir
uint32_t _sdcard_fat_rootDirStart;       // root start block for FAT16, cluster for FAT32

BOOL _sdcard_fat_read_volume(int part);
BOOL _sdcard_fat_read(uint32_t block, int action);
BOOL _sdcard_fat_cacheFlush();

BOOL sdcard_fat_setup() {
  printf("BEGIN SDCard Fat Setup\n");
  _sdcard_fat_bufferBlockNumber = 0XFFFFFFFF;
  _sdcard_fat_bufferDirty = 0;
  _sdcard_fat_bufferMirrorBlock = 0;

  if (!_sdcard_fat_read_volume(1)) {
    if (!_sdcard_fat_read_volume(0)) {
      return FALSE;
    }
  }
  printf("END SDCard Fat Setup\n");
  return TRUE;
}

BOOL _sdcard_fat_read_volume(int part) {
  uint32_t volumeStartBlock = 0;

  if (part > 4) {
    return FALSE;
  }

  // if part == 0 assume super floppy with FAT boot sector in block zero
  // if part > 0 assume mbr volume with partition table
  if (part) {
    if (!_sdcard_fat_read(volumeStartBlock, SDCARD_FAT_CACHE_FOR_READ)) {
      return FALSE;
    }
    part_t* p = &_sdcard_fat_buffer.mbr.part[part - 1];
    if ((p->boot & 0X7F) != 0 || p->totalSectors < 100 || p->firstSector == 0) {
      // not a valid partition
      return FALSE;
    }
    volumeStartBlock = p->firstSector;
  }

  if (!_sdcard_fat_read(volumeStartBlock, SDCARD_FAT_CACHE_FOR_READ)) {
    return FALSE;
  }
  bpb_t* bpb = &_sdcard_fat_buffer.fbs.bpb;
  if (bpb->bytesPerSector != 512 || bpb->fatCount == 0 || bpb->reservedSectorCount == 0 || bpb->sectorsPerCluster == 0) {
    // not valid FAT volume
    return FALSE;
  }

  _sdcard_fat_fatCount = bpb->fatCount;
  _sdcard_fat_blocksPerCluster = bpb->sectorsPerCluster;

  // determine shift that is same as multiply by blocksPerCluster_
  _sdcard_fat_clusterSizeShift = 0;
  while (_sdcard_fat_blocksPerCluster != (1 << _sdcard_fat_clusterSizeShift)) {
    // error if not power of 2
    if (_sdcard_fat_clusterSizeShift++ > 7) {
      return FALSE;
    }
  }
  _sdcard_fat_blocksPerFat = bpb->sectorsPerFat16 ? bpb->sectorsPerFat16 : bpb->sectorsPerFat32;

  _sdcard_fat_fatStartBlock = volumeStartBlock + bpb->reservedSectorCount;

  // count for FAT16 zero for FAT32
  _sdcard_fat_rootDirEntryCount = bpb->rootDirEntryCount;

  // directory start for FAT16 dataStart for FAT32
  _sdcard_fat_rootDirStart = _sdcard_fat_fatStartBlock + bpb->fatCount * _sdcard_fat_blocksPerFat;

  // data start for FAT16 and FAT32
  _sdcard_fat_dataStartBlock = _sdcard_fat_rootDirStart + ((32 * bpb->rootDirEntryCount + 511) / 512);

  // total blocks for FAT16 or FAT32
  uint32_t totalBlocks = bpb->totalSectors16 ? bpb->totalSectors16 : bpb->totalSectors32;

  // total data blocks
  _sdcard_fat_clusterCount = totalBlocks - (_sdcard_fat_dataStartBlock - volumeStartBlock);

  // divide by cluster size to get cluster count
  _sdcard_fat_clusterCount >>= _sdcard_fat_clusterSizeShift;

  // FAT type is determined by cluster count
  if (_sdcard_fat_clusterCount < 4085) {
    _sdcard_fat_fatType = 12;
  } else if (_sdcard_fat_clusterCount < 65525) {
    _sdcard_fat_fatType = 16;
  } else {
    _sdcard_fat_rootDirStart = bpb->fat32RootCluster;
    _sdcard_fat_fatType = 32;
  }

  printf("blocksPerFat: %lu\n", _sdcard_fat_blocksPerFat);
  printf("fatStartBlock: %lu\n", _sdcard_fat_fatStartBlock);
  printf("rootDirEntryCount: %u\n", _sdcard_fat_rootDirEntryCount);
  printf("rootDirStart: %lu\n", _sdcard_fat_rootDirStart);
  printf("dataStartBlock: %lu\n", _sdcard_fat_dataStartBlock);
  printf("totalBlocks: %lu\n", totalBlocks);
  printf("clusterCount: %lu\n", _sdcard_fat_clusterCount);
  printf("fatType: %u\n", _sdcard_fat_fatType);

  return TRUE;
}

int sdcard_fat_open(SDCARD_FAT_FILE* f, const char* fileName, int access) {
  printf("TODO: sdcard_fat_open\n");
  return 0;
}

void sdcard_fat_close(SDCARD_FAT_FILE* f) {
}

BOOL _sdcard_fat_cacheFlush() {
  if (_sdcard_fat_bufferDirty) {
    if (!sdcard_write_block(_sdcard_fat_bufferBlockNumber, _sdcard_fat_buffer.data)) {
      return FALSE;
    }
    // mirror FAT tables
    if (_sdcard_fat_bufferMirrorBlock) {
      if (!sdcard_write_block(_sdcard_fat_bufferBlockNumber, _sdcard_fat_buffer.data)) {
        return FALSE;
      }
      _sdcard_fat_bufferMirrorBlock = 0;
    }
    _sdcard_fat_bufferDirty = 0;
  }
  return TRUE;
}

BOOL _sdcard_fat_read(uint32_t blockNumber, int action) {
  if (_sdcard_fat_bufferBlockNumber != blockNumber) {
    if (!_sdcard_fat_cacheFlush()) {
      return FALSE;
    }
    if (!sdcard_read_block(blockNumber, _sdcard_fat_buffer.data)) {
      return FALSE;
    }
    _sdcard_fat_bufferBlockNumber = blockNumber;
  }
  _sdcard_fat_bufferDirty |= action;
  return TRUE;
}
