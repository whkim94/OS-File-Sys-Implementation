#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define _UTHREAD_PRIVATE
#include "disk.h"
#include "fs.h"

#define FAT_EOC 0xffff


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
	uint32_t fileSize;
	uint16_t fileIndex;
	uint8_t padding[10];
} __attribute__((packed));

struct FATEntry {
	int16_t data[2048];
} __attribute__((packed));

struct fileDescriptor{
	int opened; 
	int fdCount; 
	int fileD[32]; // its file descriptor
	char fileName[32][16];
	int SeekPos[32];
};


struct rootDirectory* myRootDirectory;
struct superblock* mysuperblock;
struct FATEntry* myFAT;
void* myData;
struct fileDescriptor* FD;



int fs_mount(const char *diskname)
{
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
	myRootDirectory = (struct rootDirectory*)calloc(FS_FILE_MAX_COUNT, sizeof(struct rootDirectory));
	int rootPos = 1+FATSize;
	block_read(rootPos,myRootDirectory);
	
	//reading data
	int dataSize = mysuperblock->numData;
	myData = calloc(4096, dataSize);
	int dataPos = rootPos + 1;
	for (int i=0;i<dataSize;i++) {
		block_read(dataPos+i, myData+(i*4096));
	}
	//Initiallize the file descriptor sturct
	FD = calloc(1, sizeof(struct fileDescriptor));
	FD->opened = 0;
	FD->fdCount = 1;

	return 0;
}

int fs_umount(void)
{
	if(FD->opened > 0)
		return -1;

	//reading superblock
	block_write(0,mysuperblock);
	
    //reading FAT
	int FATSize = mysuperblock->numFAT;
	for (int i=0;i<FATSize;i++) {
		block_write(1+i,myFAT+(i*4096));	
	}

	//reading RootDirectory
	int rootPos = 1+FATSize;
	block_write(rootPos,myRootDirectory);
	
	//reading data
	int dataSize = mysuperblock->numData;
	int dataPos = rootPos + 1;
	for (int i=0;i<dataSize;i++) {
		block_write(dataPos+i, myData+(i*4096));
	}

	//free structures
	free(mysuperblock);
	free(myFAT);
	free(myRootDirectory);
	free(myData);
	free(FD);
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

	int FATCount = 1;
	while (myFAT->data[FATCount] != 0) {
		FATCount++;
	}
	printf("fat_free_ratio=%d/%d\n", mysuperblock->numData-FATCount, mysuperblock->numData);

	int RDCount = 0, comp = 0;
	while ( ((myRootDirectory+RDCount)->fileName[0]) != 0 ) {
		RDCount++;
	}
	printf("rdir_free_ratio=%d/%d\n", FS_FILE_MAX_COUNT - RDCount ,FS_FILE_MAX_COUNT);
	return 0;
}

int fs_create(const char *filename)
{
	// Check if the 128th entry in rootDirectory is already full.
	uint8_t *mask = calloc(1,16);
	if ( memcmp(mask,((myRootDirectory+127)->fileName),16)!=0 )  return -1;

	//check if @filename length exceeds %FS_FILENAME_LEN characters
	if (strlen(filename)>FS_FILENAME_LEN) 	return -1;
	
	//check if @filename already exists in the system
	int tempi = strlen(filename);
	int index = -1;
	for (int i=0;i<FS_FILE_MAX_COUNT;i++) {
		if ( memcmp((myRootDirectory+i), filename, tempi) == 0)  return -1; //found a match
	}
	
	//update RD
	int freeRD = 0;
	while ( ((myRootDirectory+freeRD)->fileName[0]) != 0 )	freeRD++;

	int size = sizeof(*filename)/sizeof(char);

	for (int i=0;filename[i]!=0;i++) {
		((myRootDirectory+(freeRD))->fileName[i]) = (uint8_t)filename[i];
		tempi = i;
	}
	((myRootDirectory+(freeRD))->fileName[tempi+1]) = (uint8_t)'\0';
	((myRootDirectory+(freeRD))->fileIndex) = FAT_EOC;
	return 0;
}

int fs_delete(const char *filename)
{
	if(FD->opened > FS_OPEN_MAX_COUNT) return -1;

	//try to find the index of the filename in RD
	int tempi = strlen(filename);
	int index = -1;
	for (int i=0;i<FS_FILE_MAX_COUNT;i++) {
		if ( memcmp((myRootDirectory+i)->fileName, filename, tempi) == 0) { //found a match
			index = i;
			break;
		}
	}

	//check if there is no file named @filename
	if (index == -1)	return -1; 

	//update FAT
	int tempa = (myRootDirectory+index)->fileIndex;
	int FATIndex = tempa;
	while( (FATIndex!=0xffff) && (myFAT->data[FATIndex] != '\0')) {
		int tempIndex = myFAT->data[FATIndex];
		myFAT->data[FATIndex] = '\0';
		FATIndex = tempIndex;
	}

	//update RD
	*((myRootDirectory+(index*32))->fileName) = 0;

	uint8_t *mask = calloc(1,16);
	char* emptyFileName = 0;
	memcpy(((myRootDirectory+index)->fileName), mask, 16);
	
	mask = calloc(1,4);
	(myRootDirectory+index)->fileSize = 0;

	return 0;
}

int fs_ls(void)
{
	printf("FS Ls:\n");
	int RDIndex = 0;
	while ( ((myRootDirectory+RDIndex)->fileName[0]) != 0 ){
		printf("file: %s, ", ((myRootDirectory+RDIndex)->fileName) );
		printf("size: %d, ", ((myRootDirectory+RDIndex)->fileSize));
		printf("data_blk: %d\n", ((myRootDirectory+RDIndex)->fileIndex));
		RDIndex++;
	}
	return 0;
}

int fs_open(const char *filename)
{
	//check if we've already opened max amount files
	if(FD->opened > FS_OPEN_MAX_COUNT) return -1;

	int index = -1;
	int tempi = strlen(filename);
	for (int i=0;i<128;i++) {
		if ( memcmp((myRootDirectory+i)->fileName, filename, tempi) == 0) { //found a match
			index = i;
			break;
		}
	}

	//check if there is no file named @filename
	if (index == -1) return -1; 
	
	int i = 0;
	for(i;i < FS_OPEN_MAX_COUNT; i++){
		if(FD->fileD[i] == 0){
			FD->fileD[i] = FD->fdCount;
			break;
		}
	}

	//update file descriptors
	FD->SeekPos[i] = 0;
	memcpy(FD->fileName[i], filename, tempi);
	FD->fdCount++;
	FD->opened++;
	return FD->fileD[i];
}

int fs_close(int fd)
{
	int i = 0;
	for(i; i < FS_OPEN_MAX_COUNT; i++){
		if(FD->fileD[i] == fd)
			break;
	}
	
	//check if fd is invalid
	if(FD->fileD[i] != fd) return -1;

	//update dile descriptors
	FD->fileD[i] = 0;
	FD->SeekPos[i] = 0;
	FD->opened--;
	return 0;
}

int fs_stat(int fd)
{
	int i = 0;
	for(i; i < FS_OPEN_MAX_COUNT; i++){
		if(FD->fileD[i] == fd)
			break;
	}
	
	//check if file descriptor is invalid
	if(FD->fileD[i] != fd) return -1;
	
	char *name = FD->fileName[i];
	int strLength = strlen(FD->fileName[i]);
	int j = 0;

	for (j;i<FS_FILE_MAX_COUNT;j++) {
		if ( memcmp((myRootDirectory+j), name, strLength) == 0) { //found a match
			int size = (myRootDirectory+j)->fileSize;
			return size;
		}
	}
	return -1;
}

int fs_lseek(int fd, size_t offset)
{
	int i = 0;
	for(i; i < FS_OPEN_MAX_COUNT; i++){
		if(FD->fileD[i] == fd)
			break;
	}
	if(FD->fileD[i] != fd) return -1;
	FD->SeekPos[i] = offset;

	return 0;
}

int fs_read(int fd, void *buf, size_t count)
{
	/**
 * fs_read - Read from a file
 * @fd: File descriptor
 * @buf: Data buffer to be filled with data
 * @count: Number of bytes of data to be read
 *
 * Attempt to read @count bytes of data from the file referenced by file
 * descriptor @fd into buffer pointer by @buf. It is assumed that @buf is large
 * enough to hold at least @count bytes.
 *
 * The number of bytes read can be smaller than @count if there are less than
 * @count bytes until the end of the file (it can even be 0 if the file offset
 * is at the end of the file). The file offset of the file descriptor is
 * implicitly incremented by the number of bytes that were actually read.
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). Otherwise return the number of bytes actually read.
 */
	return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
	/**
 * fs_write - Write to a file
 * @fd: File descriptor
 * @buf: Data buffer to write in the file
 * @count: Number of bytes of data to be written
 *
 * Attempt to write @count bytes of data from buffer pointer by @buf into the
 * file referenced by file descriptor @fd. It is assumed that @buf holds at
 * least @count bytes.
 *
 * When the function attempts to write past the end of the file, the file is
 * automatically extended to hold the additional bytes. If the underlying disk
 * runs out of space while performing a write operation, fs_write() should write
 * as many bytes as possible. The number of written bytes can therefore be
 * smaller than @count (it can even be 0 if there is no more space on disk).
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). Otherwise return the number of bytes actually written.
 */
	return 0;
}