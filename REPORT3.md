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

### Phase 3: File descriptor operations

### Phase 4: File reading/writing
