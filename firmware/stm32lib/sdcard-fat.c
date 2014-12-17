// Adapted from https://github.com/adafruit/SD

#include <stdio.h>
#include <string.h>
#include "sdcard.h"
#include "sdcard-fat.h"
#include "sdcard-fat-structs.h"
#include "util.h"

#define CACHE_FOR_READ        1
#define CACHE_FOR_WRITE       2

#define FAT_FILE_TYPE_CLOSED  0
#define FAT_FILE_TYPE_NORMAL  1
#define FAT_FILE_TYPE_ROOT16  2
#define FAT_FILE_TYPE_ROOT32  3
#define FAT_FILE_TYPE_SUBDIR  4
#define FAT_FILE_TYPE_MIN_DIR FAT_FILE_TYPE_ROOT16

/** Default date for file timestamps is 1 Jan 2000 */
uint16_t const FAT_DEFAULT_DATE = ((2000 - 1980) << 9) | (1 << 5) | 1;

/** Default time for file timestamp is 1 am */
uint16_t const FAT_DEFAULT_TIME = (1 << 11);

// available bits
static uint8_t const F_UNUSED = 0X30;
// use unbuffered SD read
static uint8_t const F_FILE_UNBUFFERED_READ = 0X40;
// sync of directory entry required
static uint8_t const F_FILE_DIR_DIRTY = 0X80;

#define IS_CLOSED(f)     ( (f)->type == FAT_FILE_TYPE_CLOSED )
#define IS_ROOT(f)       ( ((f)->type == FAT_FILE_TYPE_ROOT16) || ((f)->type == FAT_FILE_TYPE_ROOT32) )
#define IS_DIR(f)        ( (f)->type >= FAT_FILE_TYPE_MIN_DIR )
#define IS_FILE(f)       ( (f)->type == FAT_FILE_TYPE_NORMAL )
#define DIR_IS_FILE(f)   ( ((f)->attributes & DIR_ATT_FILE_TYPE_MASK) == 0 )
#define DIR_IS_SUBDIR(f) ( ((f)->attributes & DIR_ATT_FILE_TYPE_MASK) == DIR_ATT_DIRECTORY )

typedef union {
  uint8_t  data[512]; // Used to access cached file data blocks.
  uint16_t fat16[256]; // Used to access cached FAT16 entries.
  uint32_t fat32[128]; // Used to access cached FAT32 entries.
  dir_t    dir[16]; // Used to access cached directory entries.
  mbr_t    mbr; // Used to access a cached MasterBoot Record.
  fbs_t    fbs; // Used to access to a cached FAT boot sector.
} SdcardCache;

SdcardCache _sdcard_fat_cache;
uint32_t _sdcard_fat_cacheBlockNumber;
uint8_t _sdcard_fat_cacheDirty;
uint32_t _sdcard_fat_cacheMirrorBlock;  // block number for mirror FAT
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
SdcardFatFile _sdcard_fat_root;

BOOL _sdcard_fat_readVolume(int part);
BOOL _sdcard_fat_cacheRead(uint32_t block, int action);
BOOL _sdcard_fat_cacheFlush();
BOOL _sdcard_fat_get(uint32_t cluster, uint32_t* value);
BOOL _sdcard_fat_put(uint32_t cluster, uint32_t value);
BOOL _sdcard_fat_fatPutEOC(uint32_t cluster);
BOOL _sdcard_fat_make83Name(const char* fileName, char* name);
BOOL _sdcard_fat_chainSize(uint32_t cluster, uint32_t* size);
BOOL _sdcard_fat_isEOC(uint32_t cluster);
uint8_t _sdcard_fat_blockOfCluster(uint32_t position);
uint32_t _sdcard_fat_clusterStartBlock(uint32_t cluster);
BOOL _sdcard_fat_cacheRawBlock(uint32_t blockNumber, uint8_t action);
BOOL _sdcard_fat_freeChain(uint32_t cluster);
BOOL _sdcard_fat_allocContiguous(uint32_t count, uint32_t* curCluster);
BOOL _sdcard_fat_cacheZeroBlock(uint32_t blockNumber);

BOOL _sdcard_fat_file_get_parent_dir(SdcardFatFile* f, const char* filePath, int* pathIdx);
BOOL _sdcard_fat_file_open_file(SdcardFatFile* f, SdcardFatFile* parentDir, const char* fileName, int mode);
dir_t* _sdcard_fat_file_readDirCache(SdcardFatFile* f);
int16_t _sdcard_fat_file_read_byte(SdcardFatFile* f);
int16_t _sdcard_fat_file_read(SdcardFatFile* f, uint8_t* buf, uint16_t nbyte);
dir_t* _sdcard_fat_file_cacheDirEntry(SdcardFatFile* f, uint8_t action);
uint8_t _sdcard_fat_file_addDirCluster(SdcardFatFile* f);
BOOL _sdcard_fat_file_truncate(SdcardFatFile* f, uint32_t length);
BOOL _sdcard_fat_file_sync(SdcardFatFile* f);

BOOL sdcard_fat_setup() {
  printf("BEGIN SDCard Fat Setup\n");
  _sdcard_fat_cacheBlockNumber = 0XFFFFFFFF;
  _sdcard_fat_cacheDirty = 0;
  _sdcard_fat_cacheMirrorBlock = 0;

  if (!_sdcard_fat_readVolume(1)) {
    if (!_sdcard_fat_readVolume(0)) {
      return FALSE;
    }
  }
  printf("END SDCard Fat Setup\n");
  return TRUE;
}

BOOL _sdcard_fat_readVolume(int part) {
  uint32_t volumeStartBlock = 0;

  if (part > 4) {
    return FALSE;
  }

  // if part == 0 assume super floppy with FAT boot sector in block zero
  // if part > 0 assume mbr volume with partition table
  if (part) {
    if (!_sdcard_fat_cacheRead(volumeStartBlock, CACHE_FOR_READ)) {
      return FALSE;
    }
    part_t* p = &_sdcard_fat_cache.mbr.part[part - 1];
    if ((p->boot & 0X7F) != 0 || p->totalSectors < 100 || p->firstSector == 0) {
      // not a valid partition
      return FALSE;
    }
    volumeStartBlock = p->firstSector;
  }

  if (!_sdcard_fat_cacheRead(volumeStartBlock, CACHE_FOR_READ)) {
    return FALSE;
  }

  bpb_t* bpb = &_sdcard_fat_cache.fbs.bpb;
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

BOOL _sdcard_fat_file_get_parent_dir(SdcardFatFile* f, const char* filePath, int* pathIdx) {
  SdcardFatFile* parent = &_sdcard_fat_root; // start with the mostparent, root!
  SdcardFatFile* subdir;
  SdcardFatFile* t;
  char subDirName[13];
  uint8_t idx;
  const char* originalPath = filePath;

  while (strchr(filePath, '/')) {
    // get rid of leading /'s
    if (filePath[0] == '/') {
      filePath++;
      continue;
    }

    if (!strchr(filePath, '/')) {
      // it was in the root directory, so leave now
      break;
    }

    // extract just the name of the next subdirectory
    idx = strchr(filePath, '/') - filePath;
    if (idx > 12) {
      idx = 12; // dont let them specify long names
    }
    strncpy(subDirName, filePath, idx);
    subDirName[idx] = 0;

    // close the subdir (we reuse them) if open
    sdcard_fat_close(subdir);
    if (!sdcard_fat_open(parent, subDirName, O_RDONLY)) {
      return FALSE;
    }

    // move forward to the next subdirectory
    filePath += idx;

    // we reuse the objects, close it.
    sdcard_fat_close(parent);

    // swap the pointers
    t = parent;
    parent = subdir;
    subdir = t;
  }

  *pathIdx = (int)(filePath - originalPath);

  memcpy(f, parent, sizeof(SdcardFatFile));
  return TRUE;
}

BOOL _sdcard_fat_get(uint32_t cluster, uint32_t* value) {
  if (cluster > (_sdcard_fat_clusterCount + 1)) {
    return FALSE;
  }

  uint32_t lba = _sdcard_fat_fatStartBlock;
  lba += _sdcard_fat_fatType == 16 ? cluster >> 8 : cluster >> 7;
  if (lba != _sdcard_fat_cacheBlockNumber) {
    if (!_sdcard_fat_cacheRead(lba, CACHE_FOR_READ)) {
      return FALSE;
    }
  }
  if (_sdcard_fat_fatType == 16) {
    *value = _sdcard_fat_cache.fat16[cluster & 0XFF];
  } else {
    *value = _sdcard_fat_cache.fat32[cluster & 0X7F] & FAT32MASK;
  }
  return TRUE;
}

BOOL sdcard_fat_seek(SdcardFatFile* f, uint32_t pos) {
  // error if file not open or seek past end of file
  if (IS_CLOSED(f) || (pos > f->fileSize)) {
    return FALSE;
  }

  if (f->type == FAT_FILE_TYPE_ROOT16) {
    f->currentPosition = pos;
    return TRUE;
  }

  if (pos == 0) {
    // set position to start of file
    f->currentCluster = 0;
    f->currentPosition = 0;
    return TRUE;
  }

  // calculate cluster index for cur and new position
  uint32_t nCur = (f->currentPosition - 1) >> (_sdcard_fat_clusterSizeShift + 9);
  uint32_t nNew = (pos - 1) >> (_sdcard_fat_clusterSizeShift + 9);

  if (nNew < nCur || f->currentPosition == 0) {
    // must follow chain from first cluster
    f->currentCluster = f->firstCluster;
  } else {
    // advance from curPosition
    nNew -= nCur;
  }

  while (nNew--) {
    if (!_sdcard_fat_get(f->currentCluster, &f->currentCluster)) {
      return FALSE;
    }
  }

  f->currentPosition = pos;
  return TRUE;
}

BOOL _sdcard_fat_make83Name(const char* fileName, char* name) {
  uint8_t c;
  uint8_t n = 7;  // max index for part before dot
  uint8_t i = 0;
  // blank fill name and extension
  while (i < 11) {
    name[i++] = ' ';
  }
  i = 0;
  while ((c = *fileName++) != '\0') {
    if (c == '.') {
      if (n == 10) {
        return FALSE; // only one dot allowed
      }
      n = 10;  // max index for full 8.3 name
      i = 8;   // place for extension
    } else {
      // illegal FAT characters
      uint8_t b;
      const uint8_t valid[] = "|<>^+=?/[];,*\"\\";
      const uint8_t* p = valid;
      while ((b = *p++)) {
        if (b == c) {
          return FALSE;
        }
      }

      // check size and only allow ASCII printable characters
      if (i > n || c < 0X21 || c > 0X7E) {
        return FALSE;
      }

      // only upper case allowed in 8.3 names - convert lower to upper
      name[i++] = c < 'a' || c > 'z' ?  c : c + ('A' - 'a');
    }
  }

  // must have a file name, extension is optional
  return name[0] != ' ';
}

BOOL _sdcard_fat_isEOC(uint32_t cluster) {
  return cluster >= (_sdcard_fat_fatType == 16 ? FAT16EOC_MIN : FAT32EOC_MIN);
}

BOOL _sdcard_fat_chainSize(uint32_t cluster, uint32_t* size) {
  uint32_t s = 0;
  do {
    if (!_sdcard_fat_get(cluster, &cluster)) {
      return FALSE;
    }
    s += 512UL << _sdcard_fat_clusterSizeShift;
  } while (!_sdcard_fat_isEOC(cluster));
  *size = s;
  return TRUE;
}

BOOL _sdcard_fat_openCachedEntry(SdcardFatFile* f, uint8_t dirIndex, uint8_t mode) {
  // location of entry in cache
  dir_t* p = _sdcard_fat_cache.dir + dirIndex;

  // write or truncate is an error for a directory or read-only file
  if (p->attributes & (DIR_ATT_READ_ONLY | DIR_ATT_DIRECTORY)) {
    if (mode & (O_WRITE | O_TRUNC)) {
      return FALSE;
    }
  }

  // remember location of directory entry on SD
  f->dirIndex = dirIndex;
  f->dirBlock = _sdcard_fat_cacheBlockNumber;

  // copy first cluster number for directory fields
  f->firstCluster = (uint32_t)p->firstClusterHigh << 16;
  f->firstCluster |= p->firstClusterLow;

  // make sure it is a normal file or subdirectory
  if (DIR_IS_FILE(p)) {
    f->fileSize = p->fileSize;
    f->type = FAT_FILE_TYPE_NORMAL;
  } else if (DIR_IS_SUBDIR(p)) {
    if (!_sdcard_fat_chainSize(f->firstCluster, &f->fileSize)) {
      return FALSE;
    }
    f->type = FAT_FILE_TYPE_SUBDIR;
  } else {
    return FALSE;
  }
  // save open flags for read/write
  f->flags = mode & (O_ACCMODE | O_SYNC | O_APPEND);

  // set to start of file
  f->currentCluster = 0;
  f->currentPosition = 0;

  // truncate file to zero length if requested
  if (mode & O_TRUNC) {
    return _sdcard_fat_file_truncate(f, 0);
  }
  return TRUE;
}

uint8_t _sdcard_fat_blockOfCluster(uint32_t position) {
  return (position >> 9) & (_sdcard_fat_blocksPerCluster - 1);
}

uint32_t _sdcard_fat_clusterStartBlock(uint32_t cluster) {
  return _sdcard_fat_dataStartBlock + ((cluster - 2) << _sdcard_fat_clusterSizeShift);
}

BOOL _sdcard_fat_cacheRawBlock(uint32_t blockNumber, uint8_t action) {
  if (_sdcard_fat_cacheBlockNumber != blockNumber) {
    if (!_sdcard_fat_cacheFlush()) {
      return FALSE;
    }
    if (!sdcard_readBlock(blockNumber, _sdcard_fat_cache.data)) {
      return FALSE;
    }
    _sdcard_fat_cacheBlockNumber = blockNumber;
  }
  _sdcard_fat_cacheDirty |= action;
  return TRUE;
}

int16_t _sdcard_fat_file_read(SdcardFatFile* f, uint8_t* buf, uint16_t nbyte) {
  // error if not open or write only
  if (IS_CLOSED(f) || !(f->flags & O_READ)) {
    return -1;
  }

  // max bytes left in file
  if (nbyte > (f->fileSize - f->currentPosition)) {
    nbyte = f->fileSize - f->currentPosition;
  }

  // amount left to read
  uint16_t toRead = nbyte;
  while (toRead > 0) {
    uint32_t block;  // raw device block number
    uint16_t offset = f->currentPosition & 0X1FF;  // offset in block
    if (f->type == FAT_FILE_TYPE_ROOT16) {
      block = _sdcard_fat_rootDirStart + (f->currentPosition >> 9);
    } else {
      uint8_t blockOfCluster = _sdcard_fat_blockOfCluster(f->currentPosition);
      if (offset == 0 && blockOfCluster == 0) {
        // start of new cluster
        if (f->currentPosition == 0) {
          // use first cluster in file
          f->currentCluster = f->firstCluster;
        } else {
          // get next cluster from FAT
          if (!_sdcard_fat_get(f->currentCluster, &f->currentCluster)) {
            return -1;
          }
        }
      }
      block = _sdcard_fat_clusterStartBlock(f->currentCluster) + blockOfCluster;
    }
    uint16_t n = toRead;

    // amount to be read from current block
    if (n > (512 - offset)) {
      n = 512 - offset;
    }

    // read block to cache and copy data to caller
    if (!_sdcard_fat_cacheRawBlock(block, CACHE_FOR_READ)) {
      return -1;
    }
    uint8_t* src = _sdcard_fat_cache.data + offset;
    uint8_t* end = src + n;
    while (src != end) {
      *buf++ = *src++;
    }
    f->currentPosition += n;
    toRead -= n;
  }
  return nbyte;
}

int16_t _sdcard_fat_file_read_byte(SdcardFatFile* f) {
  uint8_t b;
  return _sdcard_fat_file_read(f, &b, 1) == 1 ? b : -1;
}

//------------------------------------------------------------------------------
// Read next directory entry into the cache
// Assumes file is correctly positioned
dir_t* _sdcard_fat_file_readDirCache(SdcardFatFile* f) {
  // error if not directory
  if (!IS_DIR(f)) {
    return NULL;
  }

  // index of entry in cache
  uint8_t i = (f->currentPosition >> 5) & 0XF;

  // use read to locate and cache block
  if (_sdcard_fat_file_read_byte(f) < 0) {
    return NULL;
  }

  // advance to next entry
  f->currentPosition += 31;

  // return pointer to entry
  return (_sdcard_fat_cache.dir + i);
}

//------------------------------------------------------------------------------
// cache a file's directory entry
// return pointer to cached entry or null for failure
dir_t* _sdcard_fat_file_cacheDirEntry(SdcardFatFile* f, uint8_t action) {
  if (!_sdcard_fat_cacheRawBlock(f->dirBlock, action)) {
    return NULL;
  }
  return _sdcard_fat_cache.dir + f->dirIndex;
}

BOOL _sdcard_fat_fatPutEOC(uint32_t cluster) {
  return _sdcard_fat_put(cluster, 0x0FFFFFFF);
}

//------------------------------------------------------------------------------
// find a contiguous group of clusters
BOOL _sdcard_fat_allocContiguous(uint32_t count, uint32_t* curCluster) {
  // start of group
  uint32_t bgnCluster;

  // flag to save place to start next search
  uint8_t setStart;

  // set search start cluster
  if (*curCluster) {
    // try to make file contiguous
    bgnCluster = *curCluster + 1;

    // don't save new start location
    setStart = FALSE;
  } else {
    // start at likely place for free cluster
    bgnCluster = _sdcard_fat_allocSearchStart;

    // save next search start if one cluster
    setStart = 1 == count;
  }

  // end of group
  uint32_t endCluster = bgnCluster;

  // last cluster of FAT
  uint32_t fatEnd = _sdcard_fat_clusterCount + 1;

  // search the FAT for free clusters
  for (uint32_t n = 0;; n++, endCluster++) {
    // can't find space checked all clusters
    if (n >= _sdcard_fat_clusterCount) {
      return FALSE;
    }

    // past end - start from beginning of FAT
    if (endCluster > fatEnd) {
      bgnCluster = endCluster = 2;
    }
    uint32_t f;
    if (!_sdcard_fat_get(endCluster, &f)) {
      return FALSE;
    }

    if (f != 0) {
      // cluster in use try next cluster as bgnCluster
      bgnCluster = endCluster + 1;
    } else if ((endCluster - bgnCluster + 1) == count) {
      // done - found space
      break;
    }
  }
  // mark end of chain
  if (!_sdcard_fat_fatPutEOC(endCluster)) {
    return FALSE;
  }

  // link clusters
  while (endCluster > bgnCluster) {
    if (!_sdcard_fat_put(endCluster - 1, endCluster)) {
      return FALSE;
    }
    endCluster--;
  }
  if (*curCluster != 0) {
    // connect chains
    if (!_sdcard_fat_put(*curCluster, bgnCluster)) {
      return FALSE;
    }
  }
  // return first cluster number to caller
  *curCluster = bgnCluster;

  // remember possible next free cluster
  if (setStart) {
    _sdcard_fat_allocSearchStart = bgnCluster + 1;
  }

  return TRUE;
}

//------------------------------------------------------------------------------
// add a cluster to a file
uint8_t _sdcard_fat_addCluster(SdcardFatFile* f) {
  if (!_sdcard_fat_allocContiguous(1, &f->currentCluster)) {
    return FALSE;
  }

  // if first cluster of file link to directory entry
  if (f->firstCluster == 0) {
    f->firstCluster = f->currentCluster;
    f->flags |= F_FILE_DIR_DIRTY;
  }
  return TRUE;
}

//------------------------------------------------------------------------------
/**
 * The sync() call causes all modified data and directory fields
 * to be written to the storage device.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 * Reasons for failure include a call to sync() before a file has been
 * opened or an I/O error.
 */
BOOL _sdcard_fat_file_sync(SdcardFatFile* f) {
  // only allow open files and directories
  if (IS_CLOSED(f)) {
    return FALSE;
  }

  if (f->flags & F_FILE_DIR_DIRTY) {
    dir_t* d = _sdcard_fat_file_cacheDirEntry(f, CACHE_FOR_WRITE);
    if (!d) {
      return FALSE;
    }

    // do not set filesize for dir files
    if (!IS_DIR(f)) {
      d->fileSize = f->fileSize;
    }

    // update first cluster fields
    d->firstClusterLow = f->firstCluster & 0XFFFF;
    d->firstClusterHigh = f->firstCluster >> 16;

    // set modify time if user supplied a callback date/time function
    sdcard_fat_getTime(f, &d->lastWriteDate, &d->lastWriteTime);
    d->lastAccessDate = d->lastWriteDate;
    // clear directory dirty
    f->flags &= ~F_FILE_DIR_DIRTY;
  }
  return _sdcard_fat_cacheFlush();
}

//------------------------------------------------------------------------------
/**
 * Truncate a file to a specified length.  The current file position
 * will be maintained if it is less than or equal to \a length otherwise
 * it will be set to end of file.
 *
 * \param[in] length The desired length for the file.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 * Reasons for failure include file is read only, file is a directory,
 * \a length is greater than the current file size or an I/O error occurs.
 */
BOOL _sdcard_fat_file_truncate(SdcardFatFile* f, uint32_t length) {
// error if not a normal file or read-only
  if (!IS_FILE(f) || !(f->flags & O_WRITE)) {
    return FALSE;
  }

  // error if length is greater than current size
  if (length > f->fileSize) {
    return FALSE;
  }

  // fileSize and length are zero - nothing to do
  if (f->fileSize == 0) {
    return TRUE;
  }

  // remember position for seek after truncation
  uint32_t newPos = f->currentPosition > length ? length : f->currentPosition;

  // position to last cluster in truncated file
  if (!sdcard_fat_seek(f, length)) {
    return FALSE;
  }

  if (length == 0) {
    // free all clusters
    if (!_sdcard_fat_freeChain(f->firstCluster)) {
      return FALSE;
    }
    f->firstCluster = 0;
  } else {
    uint32_t toFree;
    if (!_sdcard_fat_get(f->currentCluster, &toFree)) {
      return FALSE;
    }

    if (!_sdcard_fat_isEOC(toFree)) {
      // free extra clusters
      if (!_sdcard_fat_freeChain(toFree)) {
        return FALSE;
      }

      // current cluster is end of chain
      if (!_sdcard_fat_fatPutEOC(f->currentCluster)) {
        return FALSE;
      }
    }
  }
  f->fileSize = length;

  // need to update directory entry
  f->flags |= F_FILE_DIR_DIRTY;

  if (!_sdcard_fat_file_sync(f)) {
    return FALSE;
  }

  // set file to correct position
  return sdcard_fat_seek(f, newPos);
}

//------------------------------------------------------------------------------
// cache a zero block for blockNumber
BOOL _sdcard_fat_cacheZeroBlock(uint32_t blockNumber) {
  if (!_sdcard_fat_cacheFlush()) {
    return FALSE;
  }

  // loop take less flash than memset(cacheBuffer_.data, 0, 512);
  for (uint16_t i = 0; i < 512; i++) {
    _sdcard_fat_cache.data[i] = 0;
  }
  _sdcard_fat_cacheBlockNumber = blockNumber;
  _sdcard_fat_cacheDirty |= CACHE_FOR_WRITE;
  return TRUE;
}

//------------------------------------------------------------------------------
// Add a cluster to a directory file and zero the cluster.
// return with first block of cluster in the cache
uint8_t _sdcard_fat_file_addDirCluster(SdcardFatFile* f) {
  if (!_sdcard_fat_addCluster(f)) {
    return FALSE;
  }

  // zero data in cluster insure first cluster is in cache
  uint32_t block = _sdcard_fat_clusterStartBlock(f->currentCluster);
  for (uint8_t i = _sdcard_fat_blocksPerCluster; i != 0; i--) {
    if (!_sdcard_fat_cacheZeroBlock(block + i - 1)) {
      return FALSE;
    }
  }
  // Increase directory file size by cluster size
  f->fileSize += 512UL << _sdcard_fat_clusterSizeShift;
  return TRUE;
}

BOOL _sdcard_fat_file_open_file(SdcardFatFile* f, SdcardFatFile* dirFile, const char* fileName, int mode) {
  char dname[11];
  dir_t* p;

  if (!IS_CLOSED(f)) {
    return FALSE;
  }

  if (!_sdcard_fat_make83Name(fileName, dname)) {
    return FALSE;
  }

  dirFile->currentCluster = 0;
  dirFile->currentPosition = 0;

  // bool for empty entry found
  BOOL emptyFound = FALSE;

  // search for file
  while (dirFile->currentPosition < dirFile->fileSize) {
    uint8_t index = 0XF & (dirFile->currentPosition >> 5);
    p = _sdcard_fat_file_readDirCache(dirFile);
    if (p == NULL) {
      return FALSE;
    }

    if (p->name[0] == DIR_NAME_FREE || p->name[0] == DIR_NAME_DELETED) {
      // remember first empty slot
      if (!emptyFound) {
        emptyFound = TRUE;
        f->dirIndex = index;
        f->dirBlock = _sdcard_fat_cacheBlockNumber;
      }
      // done if no entries follow
      if (p->name[0] == DIR_NAME_FREE) {
        break;
      }
    } else if (!memcmp(dname, p->name, 11)) {
      // don't open existing file if O_CREAT and O_EXCL
      if ((mode & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL)) {
        return FALSE;
      }

      return _sdcard_fat_openCachedEntry(f, 0XF & index, mode);
    }
  }
  // only create file if O_CREAT and O_WRITE
  if ((mode & (O_CREAT | O_WRITE)) != (O_CREAT | O_WRITE)) {
    return FALSE;
  }

  // cache found slot or add cluster if end of file
  if (emptyFound) {
    p = _sdcard_fat_file_cacheDirEntry(f, CACHE_FOR_WRITE);
    if (!p) {
      return FALSE;
    }
  } else {
    if (dirFile->type == FAT_FILE_TYPE_ROOT16) {
      return FALSE;
    }

    // add and zero cluster for dirFile - first cluster is in cache for write
    if (!_sdcard_fat_file_addDirCluster(dirFile)) {
      return FALSE;
    }

    // use first entry in cluster
    f->dirIndex = 0;
    p = _sdcard_fat_cache.dir;
  }

  // initialize as empty file
  memset(p, 0, sizeof(dir_t));
  memcpy(p->name, dname, 11);

  sdcard_fat_getTime(f, &p->creationDate, &p->creationTime);
  p->lastAccessDate = p->creationDate;
  p->lastWriteDate = p->creationDate;
  p->lastWriteTime = p->creationTime;

  // force write of entry to SD
  if (!_sdcard_fat_cacheFlush()) {
    return FALSE;
  }

  return _sdcard_fat_openCachedEntry(f, f->dirIndex, mode);
}

//------------------------------------------------------------------------------
// Store a FAT entry
BOOL _sdcard_fat_put(uint32_t cluster, uint32_t value) {
  // error if reserved cluster
  if (cluster < 2) {
    return FALSE;
  }

  // error if not in FAT
  if (cluster > (_sdcard_fat_clusterCount + 1)) {
    return FALSE;
  }

  // calculate block address for entry
  uint32_t lba = _sdcard_fat_fatStartBlock;
  lba += _sdcard_fat_fatType == 16 ? cluster >> 8 : cluster >> 7;

  if (lba != _sdcard_fat_cacheBlockNumber) {
    if (!_sdcard_fat_cacheRawBlock(lba, CACHE_FOR_READ)) {
      return FALSE;
    }
  }
  // store entry
  if (_sdcard_fat_fatType == 16) {
    _sdcard_fat_cache.fat16[cluster & 0XFF] = value;
  } else {
    _sdcard_fat_cache.fat32[cluster & 0X7F] = value;
  }
  _sdcard_fat_cacheDirty |= CACHE_FOR_WRITE;

  // mirror second FAT
  if (_sdcard_fat_fatCount > 1) {
    _sdcard_fat_cacheMirrorBlock = lba + _sdcard_fat_blocksPerFat;
  }
  return TRUE;
}

//------------------------------------------------------------------------------
// free a cluster chain
BOOL _sdcard_fat_freeChain(uint32_t cluster) {
  // clear free cluster location
  _sdcard_fat_allocSearchStart = 2;

  do {
    uint32_t next;
    if (!_sdcard_fat_get(cluster, &next)) {
      return FALSE;
    }

    // free cluster
    if (!_sdcard_fat_put(cluster, 0)) {
      return FALSE;
    }

    cluster = next;
  } while (!_sdcard_fat_isEOC(cluster));

  return TRUE;
}

BOOL sdcard_fat_open(SdcardFatFile* f, const char* filePath, int mode) {
  int pathidx;

  if (!_sdcard_fat_file_get_parent_dir(f, filePath, &pathidx)) {
    return FALSE;
  }
  filePath += pathidx;

  // it was the directory itself!
  if (!filePath[0]) {
    return TRUE;
  }

  // failed to open a subdir!
  if (IS_CLOSED(f)) {
    return FALSE;
  }

  // there is a special case for the Root directory since its a static dir
  if (IS_ROOT(f)) {
    if (!_sdcard_fat_file_open_file(f, &_sdcard_fat_root, filePath, mode)) {
      return FALSE;
    }
    // dont close the root!
  } else {
    // TODO: close parent dir
    if (!_sdcard_fat_file_open_file(f, f, filePath, mode)) {
      return FALSE;
    }
  }

  if (mode & O_APPEND) {
    sdcard_fat_seek(f, f->fileSize);
  }

  return TRUE;
}

void sdcard_fat_close(SdcardFatFile* f) {
  _sdcard_fat_cacheFlush();
}

BOOL _sdcard_fat_cacheFlush() {
  if (_sdcard_fat_cacheDirty) {
    if (!sdcard_writeBlock(_sdcard_fat_cacheBlockNumber, _sdcard_fat_cache.data)) {
      return FALSE;
    }
    // mirror FAT tables
    if (_sdcard_fat_cacheMirrorBlock) {
      if (!sdcard_writeBlock(_sdcard_fat_cacheBlockNumber, _sdcard_fat_cache.data)) {
        return FALSE;
      }
      _sdcard_fat_cacheMirrorBlock = 0;
    }
    _sdcard_fat_cacheDirty = 0;
  }
  return TRUE;
}

BOOL _sdcard_fat_cacheRead(uint32_t blockNumber, int action) {
  if (_sdcard_fat_cacheBlockNumber != blockNumber) {
    if (!_sdcard_fat_cacheFlush()) {
      return FALSE;
    }
    if (!sdcard_readBlock(blockNumber, _sdcard_fat_cache.data)) {
      return FALSE;
    }
    _sdcard_fat_cacheBlockNumber = blockNumber;
  }
  _sdcard_fat_cacheDirty |= action;
  return TRUE;
}



