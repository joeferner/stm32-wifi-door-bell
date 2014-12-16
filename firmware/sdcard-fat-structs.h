
#ifndef _SDCARD_FAT_STRUCTS_H_
#define _SDCARD_FAT_STRUCTS_H_

/**
 * \struct biosParmBlock
 *
 * \brief BIOS parameter block
 *
 *  The BIOS parameter block describes the physical layout of a FAT volume.
 */
typedef struct {
  /**
   * Count of bytes per sector. This value may take on only the
   * following values: 512, 1024, 2048 or 4096
   */
  uint16_t bytesPerSector;
  /**
   * Number of sectors per allocation unit. This value must be a
   * power of 2 that is greater than 0. The legal values are
   * 1, 2, 4, 8, 16, 32, 64, and 128.
   */
  uint8_t  sectorsPerCluster;
  /**
   * Number of sectors before the first FAT.
   * This value must not be zero.
   */
  uint16_t reservedSectorCount;
  /** The count of FAT data structures on the volume. This field should
   *  always contain the value 2 for any FAT volume of any type.
   */
  uint8_t  fatCount;
  /**
  * For FAT12 and FAT16 volumes, this field contains the count of
  * 32-byte directory entries in the root directory. For FAT32 volumes,
  * this field must be set to 0. For FAT12 and FAT16 volumes, this
  * value should always specify a count that when multiplied by 32
  * results in a multiple of bytesPerSector.  FAT16 volumes should
  * use the value 512.
  */
  uint16_t rootDirEntryCount;
  /**
   * This field is the old 16-bit total count of sectors on the volume.
   * This count includes the count of all sectors in all four regions
   * of the volume. This field can be 0; if it is 0, then totalSectors32
   * must be non-zero.  For FAT32 volumes, this field must be 0. For
   * FAT12 and FAT16 volumes, this field contains the sector count, and
   * totalSectors32 is 0 if the total sector count fits
   * (is less than 0x10000).
   */
  uint16_t totalSectors16;
  /**
   * This dates back to the old MS-DOS 1.x media determination and is
   * no longer usually used for anything.  0xF8 is the standard value
   * for fixed (non-removable) media. For removable media, 0xF0 is
   * frequently used. Legal values are 0xF0 or 0xF8-0xFF.
   */
  uint8_t  mediaType;
  /**
   * Count of sectors occupied by one FAT on FAT12/FAT16 volumes.
   * On FAT32 volumes this field must be 0, and sectorsPerFat32
   * contains the FAT size count.
   */
  uint16_t sectorsPerFat16;
  /** Sectors per track for interrupt 0x13. Not used otherwise. */
  uint16_t sectorsPerTrtack;
  /** Number of heads for interrupt 0x13.  Not used otherwise. */
  uint16_t headCount;
  /**
   * Count of hidden sectors preceding the partition that contains this
   * FAT volume. This field is generally only relevant for media
   *  visible on interrupt 0x13.
   */
  uint32_t hidddenSectors;
  /**
   * This field is the new 32-bit total count of sectors on the volume.
   * This count includes the count of all sectors in all four regions
   * of the volume.  This field can be 0; if it is 0, then
   * totalSectors16 must be non-zero.
   */
  uint32_t totalSectors32;
  /**
   * Count of sectors occupied by one FAT on FAT32 volumes.
   */
  uint32_t sectorsPerFat32;
  /**
   * This field is only defined for FAT32 media and does not exist on
   * FAT12 and FAT16 media.
   * Bits 0-3 -- Zero-based number of active FAT.
   *             Only valid if mirroring is disabled.
   * Bits 4-6 -- Reserved.
   * Bit 7	-- 0 means the FAT is mirrored at runtime into all FATs.
   *        -- 1 means only one FAT is active; it is the one referenced in bits 0-3.
   * Bits 8-15 	-- Reserved.
   */
  uint16_t fat32Flags;
  /**
   * FAT32 version. High byte is major revision number.
   * Low byte is minor revision number. Only 0.0 define.
   */
  uint16_t fat32Version;
  /**
   * Cluster number of the first cluster of the root directory for FAT32.
   * This usually 2 but not required to be 2.
   */
  uint32_t fat32RootCluster;
  /**
   * Sector number of FSINFO structure in the reserved area of the
   * FAT32 volume. Usually 1.
   */
  uint16_t fat32FSInfo;
  /**
   * If non-zero, indicates the sector number in the reserved area
   * of the volume of a copy of the boot record. Usually 6.
   * No value other than 6 is recommended.
   */
  uint16_t fat32BackBootBlock;
  /**
   * Reserved for future expansion. Code that formats FAT32 volumes
   * should always set all of the bytes of this field to 0.
   */
  uint8_t  fat32Reserved[12];
} __attribute__((packed)) bpb_t;

/**
 * \struct partitionTable
 * \brief MBR partition table entry
 *
 * A partition table entry for a MBR formatted storage device.
 * The MBR partition table has four entries.
 */
typedef struct {
  /**
   * Boot Indicator . Indicates whether the volume is the active
   * partition.  Legal values include: 0X00. Do not use for booting.
   * 0X80 Active partition.
   */
  uint8_t  boot;
  /**
    * Head part of Cylinder-head-sector address of the first block in
    * the partition. Legal values are 0-255. Only used in old PC BIOS.
    */
  uint8_t  beginHead;
  /**
   * Sector part of Cylinder-head-sector address of the first block in
   * the partition. Legal values are 1-63. Only used in old PC BIOS.
   */
  unsigned beginSector : 6;
  /** High bits cylinder for first block in partition. */
  unsigned beginCylinderHigh : 2;
  /**
   * Combine beginCylinderLow with beginCylinderHigh. Legal values
   * are 0-1023.  Only used in old PC BIOS.
   */
  uint8_t  beginCylinderLow;
  /**
   * Partition type. See defines that begin with PART_TYPE_ for
   * some Microsoft partition types.
   */
  uint8_t  type;
  /**
   * head part of cylinder-head-sector address of the last sector in the
   * partition.  Legal values are 0-255. Only used in old PC BIOS.
   */
  uint8_t  endHead;
  /**
   * Sector part of cylinder-head-sector address of the last sector in
   * the partition.  Legal values are 1-63. Only used in old PC BIOS.
   */
  unsigned endSector : 6;
  /** High bits of end cylinder */
  unsigned endCylinderHigh : 2;
  /**
   * Combine endCylinderLow with endCylinderHigh. Legal values
   * are 0-1023.  Only used in old PC BIOS.
   */
  uint8_t  endCylinderLow;
  /** Logical block address of the first block in the partition. */
  uint32_t firstSector;
  /** Length of the partition, in blocks. */
  uint32_t totalSectors;
} __attribute__((packed)) part_t;

/**
 * \struct directoryEntry
 * \brief FAT short directory entry
 *
 * Short means short 8.3 name, not the entry size.
 *
 * Date Format. A FAT directory entry date stamp is a 16-bit field that is
 * basically a date relative to the MS-DOS epoch of 01/01/1980. Here is the
 * format (bit 0 is the LSB of the 16-bit word, bit 15 is the MSB of the
 * 16-bit word):
 *
 * Bits 9-15: Count of years from 1980, valid value range 0-127
 * inclusive (1980-2107).
 *
 * Bits 5-8: Month of year, 1 = January, valid value range 1-12 inclusive.
 *
 * Bits 0-4: Day of month, valid value range 1-31 inclusive.
 *
 * Time Format. A FAT directory entry time stamp is a 16-bit field that has
 * a granularity of 2 seconds. Here is the format (bit 0 is the LSB of the
 * 16-bit word, bit 15 is the MSB of the 16-bit word).
 *
 * Bits 11-15: Hours, valid value range 0-23 inclusive.
 *
 * Bits 5-10: Minutes, valid value range 0-59 inclusive.
 *
 * Bits 0-4: 2-second count, valid value range 0-29 inclusive (0 - 58 seconds).
 *
 * The valid time range is from Midnight 00:00:00 to 23:59:58.
 */
typedef struct {
  /**
   * Short 8.3 name.
   * The first eight bytes contain the file name with blank fill.
   * The last three bytes contain the file extension with blank fill.
   */
  uint8_t  name[11];
  /** Entry attributes.
   *
   * The upper two bits of the attribute byte are reserved and should
   * always be set to 0 when a file is created and never modified or
   * looked at after that.  See defines that begin with DIR_ATT_.
   */
  uint8_t  attributes;
  /**
   * Reserved for use by Windows NT. Set value to 0 when a file is
   * created and never modify or look at it after that.
   */
  uint8_t  reservedNT;
  /**
   * The granularity of the seconds part of creationTime is 2 seconds
   * so this field is a count of tenths of a second and its valid
   * value range is 0-199 inclusive. (WHG note - seems to be hundredths)
   */
  uint8_t  creationTimeTenths;
  /** Time file was created. */
  uint16_t creationTime;
  /** Date file was created. */
  uint16_t creationDate;
  /**
   * Last access date. Note that there is no last access time, only
   * a date.  This is the date of last read or write. In the case of
   * a write, this should be set to the same date as lastWriteDate.
   */
  uint16_t lastAccessDate;
  /**
   * High word of this entry's first cluster number (always 0 for a
   * FAT12 or FAT16 volume).
   */
  uint16_t firstClusterHigh;
  /** Time of last write. File creation is considered a write. */
  uint16_t lastWriteTime;
  /** Date of last write. File creation is considered a write. */
  uint16_t lastWriteDate;
  /** Low word of this entry's first cluster number. */
  uint16_t firstClusterLow;
  /** 32-bit unsigned holding this file's size in bytes. */
  uint32_t fileSize;
} __attribute__((packed)) dir_t;

/**
 * \struct masterBootRecord
 *
 * \brief Master Boot Record
 *
 * The first block of a storage device that is formatted with a MBR.
 */
typedef struct {
  /** Code Area for master boot program. */
  uint8_t  codeArea[440];
  /** Optional WindowsNT disk signature. May contain more boot code. */
  uint32_t diskSignature;
  /** Usually zero but may be more boot code. */
  uint16_t usuallyZero;
  /** Partition tables. */
  part_t   part[4];
  /** First MBR signature byte. Must be 0X55 */
  uint8_t  mbrSig0;
  /** Second MBR signature byte. Must be 0XAA */
  uint8_t  mbrSig1;
} __attribute__((packed)) mbr_t;

/**
 * \struct fat32BootSector
 *
 * \brief Boot sector for a FAT16 or FAT32 volume.
 *
 */
typedef struct  {
  /** X86 jmp to boot program */
  uint8_t  jmpToBootCode[3];
  /** informational only - don't depend on it */
  char     oemName[8];
  /** BIOS Parameter Block */
  bpb_t    bpb;
  /** for int0x13 use value 0X80 for hard drive */
  uint8_t  driveNumber;
  /** used by Windows NT - should be zero for FAT */
  uint8_t  reserved1;
  /** 0X29 if next three fields are valid */
  uint8_t  bootSignature;
  /** usually generated by combining date and time */
  uint32_t volumeSerialNumber;
  /** should match volume label in root dir */
  char     volumeLabel[11];
  /** informational only - don't depend on it */
  char     fileSystemType[8];
  /** X86 boot code */
  uint8_t  bootCode[420];
  /** must be 0X55 */
  uint8_t  bootSectorSig0;
  /** must be 0XAA */
  uint8_t  bootSectorSig1;
} __attribute__((packed)) fbs_t;

#endif
