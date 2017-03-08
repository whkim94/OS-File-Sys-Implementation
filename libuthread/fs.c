#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _UTHREAD_PRIVATE
#include "disk.h"
#include "fs.h"

struct superblock{
	uint64_t signiture;
	uint16_t numBlocks;
	uint16_t rootIndex;
	uint16_t dataStart;
	uint16_t numData;
	uint8_t numFAT;
	uint8_t padding[4079];
} __attribute__((packed));


struct rootDirectory {
	uint8_t fileName[16];
	uint8_t fileSize[4];
	uint8_t fileIndex[2];
	uint8_t padding[10];
} __attribute__((packed));

struct FATEntry {
	int16_t data[2048];
} __attribute__((packed));


struct rootDirectory* myRootDirectory;
struct superblock* mysuperblock;
struct FATEntry* myFAT;
void* myData;

int fs_mount(const char *diskname)
{
	if(block_disk_open(diskname) == -1){
		printf("%s Diskname failed to open\n", diskname);
		return -1;
	}

	//reading superblock
	mysuperblock = malloc(sizeof(struct superblock));

	block_read(0,mysuperblock);


    //reading FAT
	int FATSize = mysuperblock->numFAT;

	myFAT = malloc(sizeof(struct FATEntry) * FATSize);
	for (int i=0;i<FATSize;i++) {
		block_read(1+i,myFAT+(i*4096));	
	}
	
	//reading RootDirectory
	myRootDirectory = malloc(sizeof(struct rootDirectory));
	int rootPos = 1+FATSize;
	block_read(rootPos,myRootDirectory);


	//reading data
	int dataSize = mysuperblock->numData;
	
	myData = calloc(dataSize,4096);
	int dataPos = rootPos + 1;
	for (int i=0;i<dataSize;i++) {
		block_read(dataPos+i,myData+(i*4096));
	}

	return 0;
}

int fs_umount(void)
{
	free(mysuperblock);
	free(myFAT);
	free(myRootDirectory);
	free(myData);

	block_disk_close();

	return 0;
}

int fs_info(void)
{
	printf("FS Info:\n");
	printf("total_blk_count=%d\n", mysuperblock->numBlocks);
	printf("fat_blk_count=%d\n", mysuperblock->numFAT);
	int rdirBlock = 1 + mysuperblock->numFAT;
	printf("rdir_blk=%d\n", rdirBlock);
	int dataBlock = rdirBlock + 1;
	printf("data_blk=%d\n", dataBlock);
	printf("data_blk_count=%d\n",mysuperblock->numData);
	printf("fat_free_ratio=%d/%d\n", 199,200);
	printf("rdir_free_ratio=%d/%d\n", 128,128);
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

