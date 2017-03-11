# Project 4

### Phase 1: Mounting/Unmounting

We made our own structs for superblock, FAT, root directory, and data. Each of them has the necessary variables inside of them accrding to the lecture slides(ex: superblock has signiture, number of total blocks, etc.)

- fs_mount():
 1. we check if we can open the disk, and return -1 on failure
 2. we read the first block from the disk into our superblock using block_read(0,mySuperblock)
 3. we get the number of FAT blocks from the superblock and read into our FAT struc useing block_read() in a for loop
 4. we read the next block into our root directory
 5. we get the number of data blocks from the superblock and read into our data struc useing block_read() in a for loop

- fs_umount():
 1. we use block_write() to write our super block into the disk
 2. we do the same for FAT, root directory, data (using for loops if necessary)
 3. we free all the structures to prevent memory leak
 4. we use block_disk_close() to close the disk
  
- fs_info():
 1. We print out most of the info by looking into superblock
 2. for ratio of free FAT and Root directory entries, we loop through each structure and see how many entries are free

### Phase 2: File creation/deletion

- fs_create():
 1. we check if the root directory is full by seeing if the last entry is not empty, return -1 if true
 2. we check if the length of filename exceeds FS_FILENAME_LEN characters, return -1 if true
 3. we check if filename is already in the system by looping through root directory, return -1 if true
 4. we add filename into root directory by finding the first free entry, and update the fileName,fileSize(0),fileIndex(FAT_EOC) of that entry
 
- fs_delete():
 1. we loop through root directory to check if there's a matching fileName, return -1 if there isn't
 2. we reset all FAT entries that file occupies by setting each FAT entry to 0 until we reach an FAT_EOC
 3. we reset the root directory entry with an empty fileName, 0 fileSize, and FAT_EOC fileIndex
 
- fs_ls():
 1. We loop through root directory and print out the info

### Phase 3: File descriptor operations

### Phase 4: File reading/writing
