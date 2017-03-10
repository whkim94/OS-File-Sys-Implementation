#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
	/**
 * fs_mount - Mount a file system
 * @diskname: Name of the virtual disk file
 *
 * Open the virtual disk file @diskname and mount the file system that it
 * contains. A file system needs to be mounted before files can be read from it
 * with fs_read() or written to it with fs_write().
 *
 * Return: -1 if virtual disk file @diskname cannot be opened, or if no valid
 * file system can be located. 0 otherwise.
 */
	//check if we opened the disk correctly
	if(block_disk_open(diskname) == -1){
		printf("%s Diskname failed to open\n", diskname);
		return -1;
	}

	//reading superblock
	mysuperblock = malloc(sizeof(struct superblock));
	block_read(0,mysuperblock);

    //reading FAT
	int FATSize = mysuperblock->numFAT;
	myFAT = (struct FATEntry*)calloc(FATSize, sizeof(struct FATEntry));
	for (int i=0;i<FATSize;i++) {
		block_read(1+i,myFAT+(i*4096));	
	}

	//reading RootDirectory
	myRootDirectory = (struct rootDirectory*)calloc(128, sizeof(struct rootDirectory));
	int rootPos = 1+FATSize;
	block_read(rootPos,myRootDirectory);
	
	//reading data
	int dataSize = mysuperblock->numData;
	myData = calloc(4096, dataSize);
	int dataPos = rootPos + 1;
	for (int i=0;i<dataSize;i++) {
		block_read(dataPos+i, myData+(i*4096));
	}

	return 0;
}

int fs_umount(void)
{
	/**
 * fs_umount - Unmount file system
 *
 * Unmount the currently mounted file system and close the underlying virtual
 * disk file.
 *
 * Return: -1 if no underlying virtual disk was opened, or if the virtual disk
 * cannot be closed, or if there are still open file descriptors. 0 otherwise.
 */

	free(mysuperblock);
	free(myFAT);
	free(myRootDirectory);
	free(myData);
	block_disk_close();

	return 0;
}

int fs_info(void)
{
	/**
 * fs_info - Display information about file system
 *
 * Display some information about the currently mounted file system.
 *
 * Return: -1 if no underlying virtual disk was opened. 0 otherwise.
 */
	printf("FS Info:\n");
	printf("total_blk_count=%d\n", mysuperblock->numBlocks);
	printf("fat_blk_count=%d\n", mysuperblock->numFAT);
	int rdirBlock = 1 + mysuperblock->numFAT;
	printf("rdir_blk=%d\n", rdirBlock);
	int dataBlock = rdirBlock + 1;
	printf("data_blk=%d\n", dataBlock);
	printf("data_blk_count=%d\n",mysuperblock->numData);

	int FATCount = 1;
	//printf("First entry in myFAT is: %d\n",myFAT->data[FATCount]);
	while (myFAT->data[FATCount] != 0) {
		FATCount++;
	}
	printf("fat_free_ratio=%d/%d\n", mysuperblock->numData-FATCount, mysuperblock->numData);

	int RDCount = 0, comp = 0;
	//comp = strcmp( (char)*(myRootDirectory + count)->fileName, "\0");
	//printf("First entry in myRootDirectory is: %s\n", *(myRootDirectory+RDCount*32)->fileName);
	while ( *(myRootDirectory+RDCount*32)->fileName != 0 ) {
		RDCount++;
	}
	printf("rdir_free_ratio=%d/%d\n", 128 - RDCount ,128);
	return 0;
}

int fs_create(const char *filename)
{
	
/**
 * fs_create - Create a new file
 * @filename: File name
 *
 * Create a new and empty file named @filename in the root directory of the
 * mounted file system. String @filename must be NULL-terminated and its total
 * length cannot exceed %FS_FILENAME_LEN characters (including the NULL
 * character).
 *
 * Return: -1 if a file named @filename already exists, or if string @filename
 * is too long, or if the root directory already contains %FS_FILE_MAX_COUNT
 * files. 0 otherwise.
 */
	
	// Check if the 128th entry in rootDirectory is already full.
	uint8_t *mask = calloc(1,16);
	printf("check for full RD\n");
	if ( memcpy(mask,((myRootDirectory+127)->fileName),16)==0 ) {
	//if(((myRootDirectory+127)->fileName) != 0) {
		printf("RD is full\n");
		return -1;
	}

	//check if @filename length exceeds %FS_FILENAME_LEN characters
	printf("check filename len\n");
	if (strlen(filename)>FS_FILENAME_LEN) {
		printf("filename has exceeded 16 characters\n");
		return -1;
	}

	//check if @filename already exists in the system
	printf("check already exists\n");
	int i = 0;
	for (i=0;i<128;i++) {
		if ( memcmp((myRootDirectory+i), filename, 16) == 0) { //found a match
			printf("filename already exists\n");
			return -1;
		}
	}

	
	//update FAT
	printf("check FAT\n");
	int FATCount = 1;
	while (myFAT->data[FATCount] != 0) {
		FATCount++;
	}

	//special case where FAT is full
	if (FATCount==127) {
		printf("Error: FAT is currently full\n");
		return -1;
	}

	myFAT->data[FATCount] = 0xffff;	

	//update RD
	printf("check RD while loop\n");
	int freeRD = 0;
	while ( memcmp(mask,((myRootDirectory+freeRD)->fileName),16) != 0 ){
		freeRD++;
	}
	printf("after while loop RD\n");
	int* temp = &FATCount;

	printf("Before reading fileName\n");
	int size = sizeof(*filename)/sizeof(char);
	memcpy(((myRootDirectory+(freeRD))->fileName), filename, size+1);
	
	printf("Before reading fileSize\n");
	int* noSize;
	*noSize = 0;
	memcpy(((myRootDirectory+(freeRD))->fileSize), noSize, 1);

	printf("Before reading fileIndex\n");
	size = sizeof(*temp)/sizeof(char);
	memcpy(((myRootDirectory+(freeRD))->fileIndex), temp, size);

	printf("After everything\n");

	return 0;
}

int fs_delete(const char *filename)
{
/**
 * fs_delete - Delete a file
 * @filename: File name
 *
 * Delete the file named @filename from the root directory of the mounted file
 * system.
 *
 * Return: -1 if there is no file named @filename to delete, or if file
 * @filename is currently open. 0 otherwise.
 */

	//try to find the index of the filename in RD
	int index = 0, i = 0;
	for (i=0;i<128;i++) {
		if ( memcmp((myRootDirectory+(i*32)), filename, 16) == 0) { //found a match
			index = i;
			break;
		}
	}

	//check if there is no file named @filename
	if (index == 0) {
		return -1; 
	}

	//check if the disk is opened
	int status = block_disk_open(filename);
	if ( status == -1) {
		return -1;
	}
	else if (status == 0) { //close it if we opened it
		block_disk_close();
	}

	//update FAT
	int* temp = 0;
	memcpy( temp, myRootDirectory+(i*32)+20, 2);
	int FATIndex = *temp;
	while(myFAT->data[FATIndex] != 0xffff) {
		int tempIndex = myFAT->data[FATIndex];
		myFAT->data[FATIndex] = 0;
		FATIndex = tempIndex;
	}
	myFAT->data[FATIndex] = 0;

	//update RD
	*((myRootDirectory+(index*32))->fileName) = 0;
	return 0;

}

int fs_ls(void)
{
/**
 * fs_ls - List files on file system
 *
 * List information about the files located in the root directory.
 *
 * Return: -1 if no underlying virtual disk was opened. 0 otherwise.
 */
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

