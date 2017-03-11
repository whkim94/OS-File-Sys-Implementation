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


	FD = calloc(1, sizeof(struct fileDescriptor));
	FD->opened = 0;
	FD->fdCount = 1;

	//block_disk_close();
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
	while ( ((myRootDirectory+127)->fileName) != 0 ) {
		RDCount++;
	}
	printf("rdir_free_ratio=%d/%d\n", FS_FILE_MAX_COUNT - RDCount ,FS_FILE_MAX_COUNT);
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
	if ( memcmp(mask,((myRootDirectory+127)->fileName),16)!=0 ) {

		printf("RD is full\n");
		return -1;
	}

	//check if @filename length exceeds %FS_FILENAME_LEN characters
	if (strlen(filename)>FS_FILENAME_LEN) {
		printf("filename has exceeded 16 characters\n");
		return -1;
	}

	//check if @filename already exists in the system
	int tempi = strlen(filename);
	printf("filename length is: %d\n",tempi);
	int index = -1;
	for (int i=0;i<FS_FILE_MAX_COUNT;i++) {
		if ( memcmp((myRootDirectory+i), filename, tempi) == 0) { //found a match
					printf("This is our file %s\n", (myRootDirectory+i)->fileName);

			printf("filename already exists\n");
			return -1;
		}
	}
	
	//update RD
	int freeRD = 0;
	while ( ((myRootDirectory+freeRD)->fileName[0]) != 0 ){
		freeRD++;
	}
	printf("freeRD is: %d\n", freeRD);

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

	if(FD->opened > FS_OPEN_MAX_COUNT) return -1;

	//try to find the index of the filename in RD
	printf("filename passed in is: %s\n", filename);
	int tempi = strlen(filename);
	int index = -1;
	for (int i=0;i<FS_FILE_MAX_COUNT;i++) {
		printf("((myRootDirectory+i)->fileName) is: %s\n", ((myRootDirectory+i)->fileName));
		if ( memcmp((myRootDirectory+i)->fileName, filename, tempi) == 0) { //found a match
			index = i;
			break;
		}
	}

	//check if there is no file named @filename
	if (index == -1) {
		printf("No such file name\n");
		return -1; 
	}

	int tempa = (myRootDirectory+index)->fileIndex;
	int FATIndex = tempa;
	//printf("%d\n", tempa);
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
/**
 * fs_ls - List files on file system
 *
 * List information about the files located in the root directory.
 *
 * Return: -1 if no underlying virtual disk was opened. 0 otherwise.
 */
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
	/**
 * fs_open - Open a file
 * @filename: File name
 *
 * Open file named @filename for reading and writing, and return the
 * corresponding file descriptor. The file descriptor is a non-negative integer
 * that is used subsequently to access the contents of the file. The file offset
 * of the file descriptor is set to 0 initially (beginning of the file). If the
 * same file is opened multiple files, fs_open() must return distinct file
 * descriptors. A maximum of %FS_OPEN_MAX_COUNT files can be open
 * simultaneously.
 *
 * Return: -1 if there is no file named @filename to open, or if there are
 * already %FS_OPEN_MAX_COUNT files currently open. 
 */
	if(FD->opened > FS_OPEN_MAX_COUNT) return -1;
	int index = -1;
	int tempi = strlen(filename);
	for (int i=0;i<128;i++) {
		//printf("((myRootDirectory+i)->fileName) is: %s\n", ((myRootDirectory+i)->fileName));
		if ( memcmp((myRootDirectory+i)->fileName, filename, tempi) == 0) { //found a match
			index = i;
			break;
		}
	}

	//check if there is no file named @filename
	if (index == -1) {
		printf("No such file name\n");
		return -1; 
	}

	int i = 0;
	for(i;i < FS_OPEN_MAX_COUNT; i++){
		if(FD->fileD[i] == 0){
			FD->fileD[i] = FD->fdCount;
			break;
		}
	}
	
	//FD->fileName[i] = filename;
	FD->SeekPos[i] = 0;
	memcpy(FD->fileName[i], filename, tempi);
	printf("Filename is %s\n", FD->fileName[i]);
	printf("File Descriptor %d\n", FD->fileD[i]);
	FD->fdCount++;
	FD->opened++;
	return FD->fileD[i];
}

int fs_close(int fd)
{
	/**
 * fs_close - Close a file
 * @fd: File descriptor
 *
 * Close file descriptor @fd.
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). 0 otherwise.
 */

	int i = 0;
	for(i; i < FS_OPEN_MAX_COUNT; i++){
		if(FD->fileD[i] == fd)
			break;
	}

	if(i == 31)
		return -1;

	FD->fileD[i] = 0;
	FD->SeekPos[i] = 0;
	FD->opened--;

	return 0;
}

int fs_stat(int fd)
{
	/**
 * fs_stat - Get file status
 * @fd: File descriptor
 *
 * Get the current size of the file pointed by file descriptor @fd.
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). Otherwise return the current size of file.
 */
	int i = 0;
	for(i; i < FS_OPEN_MAX_COUNT; i++){
		if(FD->fileD[i] == fd)
			break;
	}
	
	//FIXME: add case for unfound file descriptor
	if(FD->fileD[i] != fd) return -1;
	
	char *name = FD->fileName[i];
	int strLength = strlen(FD->fileName[i]);
	int j = 0;

	for (j;i<FS_FILE_MAX_COUNT;j++) {
		if ( memcmp((myRootDirectory+j), name, strLength) == 0) { //found a match
					int size = (myRootDirectory+j)->fileSize;

			printf("filename already exists\n");
			return size;
		}
	}


	return -1;
}

int fs_lseek(int fd, size_t offset)
{
	/**
 * fs_lseek - Set file offset
 * @fd: File descriptor
 * @offset: File offset
 *
 * Set the file offset (used for read and write operations) associated with file
 * descriptor @fd to the argument @offset. To append to a file, one can call
 * fs_lseek(fd, fs_stat(fd));
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open), or if @offset is out of bounds (beyond the end of the file). 0
 * otherwise.
 */

	int i = 0;
	for(i; i < FS_OPEN_MAX_COUNT; i++){
		if(FD->fileD[i] == fd)
			break;
	}

	if(FD->fileD[i] == fd) ;
	else return -1;
	

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