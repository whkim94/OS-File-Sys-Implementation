#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _UTHREAD_PRIVATE
#include "disk.h"
#include "fs.h"

struct superblock{
	uint8_t signiture[8];
	uint8_t numBloks[2];
	uint8_t rootIndex[2];
	uint8_t dataStart[2];
	uint8_t numData[2];
	uint8_t numFAT[1];
	uint8_t padding[4079];
} __attribute__((packed));

struct fat {
	uint16_t* fatArray;
};

struct rootDirectory {
	uint8_t fileName[16];
	uint8_t fileSize[4];
	uint8_t fileIndex[2];
	uint8_t padding[10];
};

int fs_mount(const char *diskname)
{
	/* TODO: PART 3 - Phase 1 */
}

int fs_umount(void)
{
	/* TODO: PART 3 - Phase 1 */
}

int fs_info(void)
{
	/* TODO: PART 3 - Phase 1 */
}

int fs_create(const char *filename)
{
	/* TODO: PART 3 - Phase 2 */
}

int fs_delete(const char *filename)
{
	/* TODO: PART 3 - Phase 2 */
}

int fs_ls(void)
{
	/* TODO: PART 3 - Phase 2 */
}

int fs_open(const char *filename)
{
	/* TODO: PART 3 - Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: PART 3 - Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: PART 3 - Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: PART 3 - Phase 3 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: PART 3 - Phase 4 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: PART 3 - Phase 4 */
}

