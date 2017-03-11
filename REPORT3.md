# Project 4

### Phase 1: Mounting/Unmounting

We made our own structs for superblock, FAT, root directory, and data. Each of them has the necessary variables inside of them accrding to the lecture slides(ex: superblock has signiture, number of total blocks, etc.)

- fs_mount():
 1. we check if we can open the disk using block_disk_open(diskname), and return -1 on failure
 2. we read the first block from the disk into our superblock struct using block_read(0,mySuperblock)
 3. we get the number of FAT blocks from the superblock and read into our FAT struct useing block_read() in a for loop
 4. we read the next block into our root directory
 5. we get the number of data blocks from the superblock and read into our data struct useing block_read() in a for loop

- fs_umount():
 1. we use block_write() to write our super block back into the virtual disk
 2. we do the same for FAT, root directory, data (using for loops if necessary)
 3. we free all the structures to prevent memory leak
 4. we use block_disk_close() to close the disk
  
- fs_info():
 1. we print out most of the info by looking into superblock
 2. for ratio of free entries of FAT and Root directory, we loop through each structure and see how many entries are free

### Phase 2: File creation/deletion

- fs_create():
 1. we check if the root directory is full by checking if the last entry is not empty, return -1 if true
 2. we check if the length of filename exceeds FS_FILENAME_LEN characters, return -1 if true
 3. we check if filename is already in the system by looping through root directory, return -1 if true
 4. we add filename into root directory by finding the first free entry, and update the fileName,fileSize(0),fileIndex(FAT_EOC)
 
- fs_delete():
 1. we loop through root directory to check if there's a matching fileName, return -1 if there isn't
 2. we reset all FAT entries that file occupies by setting each FAT entry to 0 until we reach an FAT_EOC
 3. we reset and update the root directory entry with an empty fileName, fileSize(0), and fileIndex(FAT_EOC)
 
- fs_ls():
 1. We loop through root directory and print out the info

### Phase 3: File descriptor operations

We created FileDescriptor struct, which has the following variables:

- opened: how many files are currently opened
- fdCount: our own file descriptor that we will be assigning
- fileD[32]: an array that records all file descriptors that are opened
- fileName[32][16]: an array that records all the filenames that are currently open
- SeekPos[32]: an array that records all seek positions of each opened file

In addition, we modified fs_delete() so that it would return -1 if there are any open files.


Given functions:

- fs_open(): 
 1. we check if there are more opened file than FS_OPEN_MAX_COUNT, return -1 if true
 2. we loop through the root directory to see if filename exists, return -1 if it doesn't
 2. we loop through the array of file descriptors to find a free spot, and find the index
 3. we set the fileName[index] to filename, fileD[index] to fdCount(later incremented), SeekPos[index] to 0
 4. we increment opened so that we know we opened another file, and increment fdCount so that we give a new file descriptor next time we open another file
 5. we return the file descriptor of the opened file

- fs_close():
 1. we loop through the array of file descriptors to find the index of the filename, return -1 if the file descriptor wasn't found
 2. we set fileD[index] to 0; SeekPos[index] to 0, and decrement opened so that we know one less file is opened
 
- fs_stat():
 1. we loop through the array of opened file descriptors to find the index of the matching one, return -1 if none was found
 2. we get the name of the index of found descriptor(fileName[index])
 3. we loop through the root directory to find a matching filename, return -1 if 
 4. we return the fileSize of the found file
 
- fs_lseek():
 1. we loop through the array of file descriptors to find the index of the filename, return -1 if the file descriptor wasn't found
 2. we set the SeekPos[index] to @offset

### Phase 4: File reading/writing

We didn't get this far.

- fs_read():
- fs_write():

### Testing

- We used the given fs.x to help us test our functions.
- We use fs.x to make a new file system, and then keep testing: test-fs.x info [diskname] until mount(), unmount(), info() are returning correct values.
- After that, we type: test-fs.x add [diskname] [filename], and use fs.x ls [diskname], to check if our fs_create() is working
- Next, we type: test-fs.x rm [diskname] [filename], and use fs.x ls [diskname], to check if our fs_delete() is working
- for phase3, we use test-fs.x cat [diskname] [filename], while printing out the file descriptors to check if they are correct
 - Note: Our file descriptor would reset every time we mount(), so we asked on piazza (https://piazza.com/class/iwmqg9yozbn4yp?cid=521), and got the responds that it was acceptable behavior.

### Resources

- http://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
- http://man7.org/linux/man-pages/man3/memcmp.3.html
- http://www.cplusplus.com/reference/cstring/strlen/
- http://stackoverflow.com/questions/2521927/initializing-a-global-struct-in-c
