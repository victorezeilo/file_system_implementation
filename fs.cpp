#include <iostream>
#include <vector>
#include <algorithm> 
#include <cstring>
#include "fs.h"


// Implement the constructor
FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";

    format(); // format the disk

    // start at the root directory
    current_directory = ROOT_BLOCK;

}

// Implement the destructor
FS::~FS()
{
    std::cout << "FS::~FS()... Destroying file system\n";
    format();
}

//----------------------------------------------------------------------
// Task 1.1: Format implementation
//----------------------------------------------------------------------
// formats the disk, i.e., creates an empty file system
//----------------------------------------------------------------------
// Block 0: Reserved for the root directory.
// Block 1: Reserved for the FAT itself.
// The rest of the blocks are free.
//----------------------------------------------------------------------
// Step 1. Define FAT Initialization
// Step 2. Reset Disk Content
// Step 3. Write FAT and Root Directory to Disk
// ---------------------------------------------------------------------
int
FS::format()
{
    // std::cout << "FS::format()\n";
    
    // Intialize the FAT, mark all blocks as free
    for (int i = 0; i < BLOCK_SIZE / 2; ++i) {
        fat[i] = FAT_FREE;
    }
    fat[current_directory] = FAT_EOF; // Reserve root directory block
    fat[FAT_BLOCK] = FAT_EOF;  // Reserve FAT block

    // clear the disk (except the FAT and Root Directory, 0 and 1)
    uint8_t clear_block[BLOCK_SIZE];
    std::memset(clear_block, 0, BLOCK_SIZE); // fill all the block with 0
    for (int i = 2; i < disk.get_no_blocks(); ++i) {
        if (disk.write(i, clear_block) != 0) {
            std::cerr << "FS::format()... Error writing block " << i << " to disk\n";
            return -1;
        }
    }

    // write the FAT to the disk
    if (write_fat_to_disk() != 0) {
        return -1;
    }

    // write the root directory to the disk
    std::memset(clear_block, 0, BLOCK_SIZE); // clear the block
    if (disk.write(current_directory, clear_block) != 0) {
        std::cerr << "FS::format()... Error writing root directory to disk\n";
        return -1;
    }

    // std::cout << "FS::format()... Disk formatted successfully\n";
    return 0;
}

//----------------------------------------------------------------------
// Task 1.2: Create implementation
//----------------------------------------------------------------------
// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
//----------------------------------------------------------------------
// Step 1. Check if the file already exists
// Step 2. Read user input
// Step 3. Allocate blocks for the file
// Step 4. Update the FAT
// Step 5. Add directory entry
// Step 6. Write the file content to the disk
// ---------------------------------------------------------------------
int FS::create(std::string filepath)
{
    // std::cout << "FS::create(" << filepath << ")\n";

    if (filepath.length() > 55) {
        std::cerr << "FS::create()... File name too long\n";
        return -1;
    }
    // check if the file already exists
    uint8_t dir_blk[BLOCK_SIZE];
    dir_entry* dir_entries = read_directory(dir_blk);
    if (!dir_entries) return -1;                         // cast the block to dir_entry

    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) { // iterate over the directory entries
        if (std::strcmp(dir_entries[i].file_name, filepath.c_str()) == 0) { // check if the file already exists
            std::cerr << "FS::create()... File already exists\n";
            return -1;
        }
    }

    // check if the directory has enough space for the new file (max 64 files)
    int file_count = 0;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) { // iterate over the directory entries
        if (dir_entries[i].file_name[0] != '\0') { // check if the entry is empty
            file_count++; // increment the file count
        }
    }

    // check if the directory has enough space
    if (file_count >= 64) {
        std::cerr << "FS::create()... Directory is full\n";
        return -1;
    }

    // read user input data
    std::cout << "Enter data for the file (end with an empty row):\n";
    std::string input_data;
    std::string input_line;
    while (std::getline(std::cin, input_line) && !input_line.empty()) {
        input_data += input_line + "\n";
        // std::cout << "input_data: " << input_data << "\n";
    }

    // // Remove the trailing newline character if it exists
    // if (!input_data.empty() && input_data.back() == '\n') {
    //     input_data.pop_back();
    // }

    // allocate blocks for the file
    int needed_blocks = (input_data.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int first_block = -1;
    int prev_block = -1;
    int current_block;
    const char* file_content = input_data.c_str();

    for (int i = 0; i < needed_blocks; ++i) { // iterate over the needed blocks

        current_block = -1; // reset the current block

        for (int j = 2; j < disk.get_no_blocks(); ++j) {    // iterate over the blocks
            if (fat[j] == FAT_FREE) {                       // check if the block is free
                current_block = j;                          // set the current block
                fat[j] = FAT_EOF;                           // mark the block as the last block

                if (prev_block != -1) {
                    fat[prev_block] = current_block;        // update the previous block
                }
                prev_block = current_block;                 // set the previous block
                break;
            }
        }
        if (current_block == -1) {                          // check if there is enough space on the disk
            std::cerr << "FS::create Not enough space on disk\n";
            return -1;
        }

        if (first_block == -1) {                            // set the first block
            first_block = current_block;                    // set the first block
        }

        // write data to the current block
        uint8_t blk[BLOCK_SIZE];
        std::memset(blk, 0, BLOCK_SIZE);                                            // clear the block
        int bytes_to_copy = std::min((int)input_data.size() - i * BLOCK_SIZE, BLOCK_SIZE);
        std::memset(blk, 0, BLOCK_SIZE); // Ensure zeroed-out memory before writing
        std::memcpy(blk, file_content + i * BLOCK_SIZE, bytes_to_copy);             // copy the data to the block

        if (disk.write(current_block, blk) != 0) {                                // write the block to the disk
            std::cerr << "FS::create()... Error writing block " << current_block << " to disk\n";
            return -1;
        }
    }

    // add directory entry
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) { // iterate over the directory entries
        if (dir_entries[i].file_name[0] == '\0') { // check if the entry is empty
            std::strncpy(dir_entries[i].file_name, filepath.c_str(), sizeof(dir_entries[i].file_name) - 1);
            dir_entries[i].size = input_data.size(); // set the file size
            dir_entries[i].first_blk = first_block; // set the first block
            dir_entries[i].type = TYPE_FILE; // set the type
            dir_entries[i].access_rights = READ | WRITE; // set the access rights
            break;
        }
    }

    // update the fat on the disk
    if (write_fat_to_disk() != 0) {
        return -1;
    }

    // write the root directory to the disk
    if (disk.write(current_directory, dir_blk) != 0) {
        std::cerr << "FS::create()... Error writing root directory to disk\n";
        return -1;
    }

    std::cout << "FS::create()... File created successfully\n";
    return 0;
}

//------------------------------------------------------------------------
// Task 1.3: cat implementation
//------------------------------------------------------------------------
// cat <filepath> reads the content of a file and prints it on the screen
//------------------------------------------------------------------------
// Step 1. Find the file in the current Directory
// Step 2. Retrieve the starting block of the file
// Step 3. Read the file content from the disk
// Step 4. Print the file content on the screen
// -----------------------------------------------------------------------
int
FS::cat(std::string filepath)
{
    // std::cout << "FS::cat(" << filepath << ")\n";

    // find the file in the current directory
    uint8_t dir_blk[BLOCK_SIZE];
    dir_entry* dir_entries = read_directory(dir_blk);
    if (!dir_entries) return -1;                         // cast the block to dir_entry

    int file_block = -1;
    int file_size = 0;
    int file_type = -1;

    // find the file in the directory
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {                              // iterate over the directory entries
        if (std::strcmp(dir_entries[i].file_name, filepath.c_str()) == 0) {                 // check if the file exists
            file_block = dir_entries[i].first_blk;                                          // get the first block
            file_size = dir_entries[i].size;                                               // get the file size
            file_type = dir_entries[i].type;                                                // get the file type
            break;
        }
    }

    if (file_block == -1) {                                                                 // check if the file exists
        std::cerr << "FS::cat()... File not found\n";
        return -1;
    }

    if (file_type == TYPE_DIR) {                                                            // check if the file is a directory
        std::cerr << "FS::cat()... Cannot cat a directory\n";
        return -1;
    }

    // read the file content from the disk
    int current_block = file_block;
    int bytes_left = file_size;
    uint8_t blk[BLOCK_SIZE];

    while (current_block != FAT_EOF) {                                                     // iterate over the blocks
        if (disk.read(current_block, blk) != 0) {                                           // read the block from the disk
            std::cerr << "FS::cat()... Error reading block " << current_block << " from disk\n";
            return -1;
        }

        int bytes_to_print = std::min(bytes_left, BLOCK_SIZE);                              // get the bytes to print
        std::cout.write(reinterpret_cast<char*>(blk), bytes_to_print);                       // print the block
        bytes_left -= bytes_to_print;                                                       // update the bytes left
        current_block = fat[current_block];                                                 // get the next block
    }

    std::cout << "\n";
    return 0;
}

//--------------------------------------------------------------------------
// Updating Task 1.4: ls implementation 
//--------------------------------------------------------------------------
// ls lists the content in the currect directory (files and sub-directories)
//--------------------------------------------------------------------------
// Step 1. Read the root directory
// Step 2. Print the directory first
// Step 3. Print the file next
// ------------------------------------------------------------------------
int
FS::ls() {
    // std::cout << "FS::ls()... Listing directory contents\n";

    // Step 1: Read the current directory
    uint8_t dir_blk[BLOCK_SIZE];
    dir_entry* dir_entries = read_directory(dir_blk);
    if (!dir_entries) return -1;

    std::cout << "name\t type\t size\n";

    // print the directory first
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {
        if (dir_entries[i].file_name[0] != '\0' && dir_entries[i].type == TYPE_DIR) { // check if the entry is a directory
            if (std::strcmp(dir_entries[i].file_name, "..") != 0) { // skip the '..' entry
                std::cout << dir_entries[i].file_name << "\t dir\t -\n"; // print the directory name
            }
        }
    }

    // print the files next
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {
        if (dir_entries[i].file_name[0] != '\0' && dir_entries[i].type == 0) { // check if the entry is a file
            std::cout << dir_entries[i].file_name << "\t file\t " << dir_entries[i].size << "\n"; // print the file name and size
        }
    }
   

    return 0;
}


//--------------------------------------------------------------------------
// Task 2.1: cp implementation
//--------------------------------------------------------------------------
// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
//--------------------------------------------------------------------------
// Step 1. Find the source file in the directory
// Step 2. Check if the destination file already exists
// Step 3. Allocate blocks for the destination file
// Step 4. Copy the file content block-by-block
// Step 5. Add directory entry for the destination file
// Step 6. Update the FAT
// ------------------------------------------------------------------------
int
FS::cp(std::string sourcefilename, std::string destfilename) {
    // std::cout << "FS::cp(" << sourcefilename << ", " << destfilename << ")...\n";

    // Read the current directory block
    uint8_t dir_blk[BLOCK_SIZE];
    dir_entry* dir_entries = read_directory(dir_blk);
    if (!dir_entries) return -1;

    // Locate the source file
    int src_index = find_entry_by_name(dir_entries, sourcefilename);
    if (src_index == -1) {
        std::cerr << "FS::cp()... Source file not found\n";
        return -1;
    }

    // Check if destination exists in current directory
    int dest_index = find_entry_by_name(dir_entries, destfilename);

    // If destfilename is a directory, move inside it!
    if (dest_index != -1 && dir_entries[dest_index].type == 1) {
       // destination is a directory
        return cp_into_dir(dir_entries[src_index], dir_entries[dest_index].first_blk);
    }

    // If destfilename is a new filename in current dir, perform regular copy
    if (dest_index != -1) {
        std::cerr << "FS::cp()... Destination file already exists\n";
        return -1;
    }

    return cp_into_dir(dir_entries[src_index], current_directory, destfilename);
}


// -----------------------------------------------------------------------
// Task 2.2: mv implementation
// -----------------------------------------------------------------------
// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
// ------------------------------------------------------------------------
// Step 1. Fetch the directory entry of the source file
// Step 2. Check if the source file exists
// Step 3. Check if the destination file already exists
// Step 4. Update the directory entry (rename the file)
// Step 5. Update the FAT (keep the content)
// ------------------------------------------------------------------------
int
FS::mv(std::string sourcepath, std::string destpath)
{
    // std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";

    // read the root directory
    uint8_t dir_blk[BLOCK_SIZE];
    dir_entry* dir_entries = read_directory(dir_blk);
    if (!dir_entries) return -1;                         // cast the block to dir_entry

    // find the source file in the directory
    int source_index = find_entry_by_name(dir_entries, sourcepath);
    if (source_index == -1) {
        return -1;
    }


    // check if the destination file already exists
    int dest_index = find_entry_by_name(dir_entries, destpath);

    // if destination file is a directory, move the source file inside it
    if (dest_index != -1 && dir_entries[dest_index].type == TYPE_DIR) {
        // mpve into the directory
        if (cp_into_dir(dir_entries[source_index], dir_entries[dest_index].first_blk) != 0) {
            std::cerr << "FS::mv()... Error moving file to directory\n";
            return -1;
        }

        // remove the source file
        std::memset(&dir_entries[source_index], 0, sizeof(dir_entry));                 // clear the source file entry

        // write the root directory to the disk
        if (disk.write(current_directory, dir_blk) != 0) {
            std::cerr << "FS::mv()... Error writing root directory to disk\n";
            return -1;
        }

        // std::cout << "FS::mv()... File moved successfully\n";
        return 0;
    }

    // if destination file does not exist, rename the source file
    if (dest_index != -1) {
        std::cerr << "FS::mv()... Destination file already exists\n";
        return -1;
    }

    // rename the file
    std::strncpy(dir_entries[source_index].file_name, destpath.c_str(), sizeof(dir_entries[source_index].file_name) - 1);

    // write the root directory to the disk
    if (disk.write(current_directory, dir_blk) != 0) {
        std::cerr << "FS::mv()... Error writing root directory to disk\n";
        return -1;
    }

    // std::cout << "FS::mv()... File moved successfully\n";
    return 0;
}

// -----------------------------------------------------------------------
// Task 2.3 rm implementation
// -----------------------------------------------------------------------
// rm <filepath> removes / deletes the file <filepath>
// ------------------------------------------------------------------------
// Step 1. Find the file in the directory
// Step 2. Mark the blocks as free in the FAT
// Step 3. Remove the directory entry
// Step 4. Update the FAT
// ------------------------------------------------------------------------
int
FS::rm(std::string filepath)
{
    //std::cout << "FS::rm(" << filepath << ")\n";

    // read the root directory
    uint8_t dir_blk[BLOCK_SIZE];
    dir_entry* dir_entries = read_directory(dir_blk);
    if (!dir_entries) return -1;                         // cast the block to dir_entry

    // find the file in the directory
    int file_index = -1;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {
        if (std::strcmp(dir_entries[i].file_name, filepath.c_str()) == 0) {
            file_index = i;
            break;
        }
    }

    if (file_index == -1) {
        std::cerr << "FS::rm()... File not found\n";
        return -1;
    }

    // mark the blocks as free in the FAT
    free_blocks(dir_entries[file_index].first_blk);

    // remove the directory entry
    std::memset(&dir_entries[file_index], 0, sizeof(dir_entry));

    // write the root directory to the disk
    if (disk.write(current_directory, dir_blk) != 0) {
        std::cerr << "FS::rm()... Error writing root directory to disk\n";
        return -1;
    }

    // update the fat on the disk
    if (write_fat_to_disk() != 0) {
        return -1;
    }

    std::cout << "FS::rm()... File removed successfully\n";
    return 0;
}

// -----------------------------------------------------------------------
// Task 2.4: append implementation
// -----------------------------------------------------------------------
// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
// ------------------------------------------------------------------------
// Step 1. Find the source file and the destination file in the directory
// Step 2. Check if the source file and the destination file exist
// Step 3. Check if destination file is not full, and has enough space
// Step 4. Read the content of the source file, and find the last block of the destination file
// Step 5. Allocate blocks for the destination file
// Step 6. Copy the file content block-by-block
// Step 7. Update the FAT
// ------------------------------------------------------------------------
 
int FS::append(std::string filepath1, std::string filepath2)
{
    // Step 1: Find the source and destination files
    dir_entry source_entry, dest_entry;
    if (find_file(filepath1, source_entry) == -1) {
        std::cerr << "FS::append()... Source file not found\n";
        return -1;
    }
    if (find_file(filepath2, dest_entry) == -1) {
        std::cerr << "FS::append()... Destination file not found\n";
        return -1;
    }

    // Ensure both are files
    if (source_entry.type != TYPE_FILE || dest_entry.type != TYPE_FILE) {
        std::cerr << "FS::append()... Invalid file type\n";
        return -1;
    }

    int source_size = source_entry.size;                                // Get source file size
    int dest_size = dest_entry.size;                                    // Get destination file size
    int total_size = source_size + dest_size;                           // Calculate total size

    // Calculate needed blocks and check free space
    int existing_blocks = (dest_size + BLOCK_SIZE - 1) / BLOCK_SIZE;    // Calculate existing blocks
    int needed_blocks = (total_size + BLOCK_SIZE - 1) / BLOCK_SIZE;     // Calculate needed blocks
    int additional_blocks = needed_blocks - existing_blocks;            // Calculate additional blocks

    int free_blocks = 0;                                                // Count free blocks
    for (int i = 2; i < disk.get_no_blocks(); ++i) {                    // Iterate over the blocks
        if (fat[i] == FAT_FREE) free_blocks++;                          // Check if the block is free
    }

    if (additional_blocks > free_blocks) {                              // Check if there is enough space
        std::cerr << "FS::append()... Not enough disk space\n";
        return -1;
    }

    // Find last block of destination and read its data
    int last_block = find_last_block(dest_entry.first_blk);

    uint8_t buffer[BLOCK_SIZE];                                     // Buffer for reading and writing data  
    int dest_offset = dest_size % BLOCK_SIZE;                       // Offset in the last block

    if (last_block != -1 && dest_offset > 0) {                      // Read last block if it exists
        disk.read(last_block, buffer);                              // Read existing data
    } else {
        memset(buffer, 0, BLOCK_SIZE);                              // Clear buffer
    }

    int source_block = source_entry.first_blk;                      // Start with the first block of source

    while (source_block != FAT_EOF) {                               // Iterate over source blocks
        uint8_t src_data[BLOCK_SIZE];
        disk.read(source_block, src_data);                          // Read source block

        int bytes_to_copy = (source_block == fat[source_block]) ? 
                            source_size % BLOCK_SIZE : BLOCK_SIZE;  // Calculate bytes to copy

        if (bytes_to_copy == 0) bytes_to_copy = BLOCK_SIZE;        // Ensure at least one block is copied

        int bytes_written = 0;
        while (bytes_written < bytes_to_copy) {                     // Copy data block-by-block
            int space_left = BLOCK_SIZE - dest_offset;
            int chunk = std::min(space_left, bytes_to_copy - bytes_written);    // Calculate chunk size

            memcpy(buffer + dest_offset, src_data + bytes_written, chunk);      // Copy data to buffer
            bytes_written += chunk;
            dest_offset += chunk;

            if (dest_offset == BLOCK_SIZE) {
                // Write full block and allocate new
                disk.write(last_block, buffer);
                int new_block = allocate_block();               // Allocate new block
                if (new_block == -1) {                          // Check if block is allocated
                    std::cerr << "FS::append()... No space\n";
                    return -1;
                }

                fat[last_block] = new_block;                    // Update FAT
                last_block = new_block;                         // Update last block
                fat[last_block] = FAT_EOF;                      // Mark as last block
                dest_offset = 0;                                // Reset offset
                memset(buffer, 0, BLOCK_SIZE);                  // Clear buffer
            }
        }

        source_block = fat[source_block];                       // Get next source block
    }

    // Write remaining data in buffer
    if (dest_offset > 0) {
        disk.write(last_block, buffer);
    }

    // Update directory entry for destination
    uint8_t dir_blk[BLOCK_SIZE];
    dir_entry* dir_entries = read_directory(dir_blk);           // Read directory
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {  // Iterate over directory entries
        if (strcmp(dir_entries[i].file_name, filepath2.c_str()) == 0) {
            dir_entries[i].size = total_size;                   // Update file size
            break;
        }
    }
    disk.write(current_directory, dir_blk);                            // Write directory to disk
    write_fat_to_disk();

    std::cout << "FS::append()... File appended successfully\n";
    return 0;
}

// -----------------------------------------------------------------------
// Task 3.1: mkdir implementation
// -----------------------------------------------------------------------
// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
// ------------------------------------------------------------------------
// Step 1. Check if the directory already exists
// Step 2. find a free directory entry
// Step 3. Allocate blocks for the directory
// Step 4. Initialize the directory
// Step 5. Add updated directory entry to the disk
// Step 6. Update the FAT and write to disk
// ------------------------------------------------------------------------ 
int
FS::mkdir(std::string dirpath)
{
    //std::cout << "FS::mkdir(" << dirpath << ")\n";

    // read the current directory
    uint8_t dir_blk[BLOCK_SIZE];
    dir_entry* dir_entries = read_directory(dir_blk);
    if (!dir_entries) return -1;                         // cast the block to dir_entry


    // check if the directory already exists
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {
        if (std::strcmp(dir_entries[i].file_name, dirpath.c_str()) == 0) {
            std::cerr << "FS::mkdir()... Directory already exists\n";
            return -1;
        }
    }

    // find a free directory entry
    int free_index = -1;
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {
        if (dir_entries[i].file_name[0] == '\0') {
            free_index = i;
            break;
        }
    }

    if (free_index == -1) {
        std::cerr << "FS::mkdir()... Directory is full\n";
        return -1;
    }

    // allocate blocks for the directory
    int new_dir_block = allocate_block();
    if (new_dir_block == -1) {
        std::cerr << "FS::mkdir()... Not enough space on disk\n";
        return -1;
    }

    // initialize the directory entry
    strncpy(dir_entries[free_index].file_name, dirpath.c_str(), sizeof(dir_entries[free_index].file_name) - 1);     // set the file name
    dir_entries[free_index].size = 0;                                                   // set the file size
    dir_entries[free_index].first_blk = new_dir_block;                                  // set the first block
    dir_entries[free_index].type = TYPE_DIR;                                            // set the type
    dir_entries[free_index].access_rights = READ | WRITE | EXECUTE;                     // set the access rights


    // initialize the new directory block with '..' entry
    uint8_t new_dir_blk[BLOCK_SIZE] = {0};                                               // create a new block
    dir_entry* new_dir_entries = reinterpret_cast<dir_entry*>(new_dir_blk);             // cast the block to dir_entry

    // set the first entry '..' to the parent directory
    strncpy(new_dir_entries[0].file_name, "..", sizeof(new_dir_entries[0].file_name) - 1); // set the file name
    new_dir_entries[0].size = 0;                                                        // set the file size
    new_dir_entries[0].first_blk = current_directory;                                          // set the first block, the parent directory
    new_dir_entries[0].type = TYPE_DIR;                                                 // set the type
    new_dir_entries[0].access_rights = READ | WRITE | EXECUTE;                          // set the access rights

    // write the updated directory entry to the disk
    if (disk.write(current_directory, dir_blk) != 0) {
        std::cerr << "FS::mkdir()... Error writing root directory to disk\n";
        return -1;
    }


    // write the new directory to the disk
    if (disk.write(new_dir_block, new_dir_blk) != 0) {
        std::cerr << "FS::mkdir()... Error writing new directory to disk\n";
        return -1;
    }


    // update the fat on the disk
    if (write_fat_to_disk() != 0) {
        return -1;
    }


    // std::cout << "FS::mkdir()... Directory created successfully\n";

    return 0;
}

// -----------------------------------------------------------------------
// Task 3.2: cd implementation
// -----------------------------------------------------------------------
// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
// ------------------------------------------------------------------------
// Step 1. Handle the special case of changing to the parent directory
// Step 2. Find the directory of the target directory
// Step 3. Update the current directory
// ------------------------------------------------------------------------
int
FS::cd(std::string dirpath)
{
    //std::cout << "FS::cd(" << dirpath << ")\n";

    // read the current directory
    uint8_t dir_blk[BLOCK_SIZE];
    dir_entry* dir_entries = read_directory(dir_blk);
    if (!dir_entries) return -1;                         // cast the block to dir_entry

    // handle the special case of changing to the parent directory
    if (dirpath == "..") {
        if (current_directory == ROOT_BLOCK) {                             // check if the current directory is the root directory
            std::cerr << "FS::cd()... Already in the root directory\n";
            return -1;
        }

        // find the parent directory
        for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {        // iterate over the directory entries
            if (std::strcmp(dir_entries[i].file_name, "..") == 0) {       // check if the entry is the parent directory
                current_directory = dir_entries[i].first_blk;             // set the current directory to the parent directory
                // std::cout << "FS::cd()... Changed to the parent directory\n";
                return 0;
            }
        }

        std::cerr << "FS::cd()... Parent directory not found\n";
        return -1;
    }

    // find the directory of the target directory
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {            // iterate over the directory entries
        if (std::strcmp(dir_entries[i].file_name, dirpath.c_str()) == 0) { // check if the file exists
            if (dir_entries[i].type != TYPE_DIR) {                         // check if the entry is a directory
                std::cerr << "FS::cd()... Not a directory\n";
                return -1;
            }

            // update the current directory
            current_directory = dir_entries[i].first_blk;                  // set the current directory
            //std::cout << "FS::cd()... Changed to the directory\n";
            return 0;
        }
    }

    std::cerr << "FS::cd()... Directory not found\n";
    return -1;
}

// -----------------------------------------------------------------------
// Task 3.3: pwd implementation
// -----------------------------------------------------------------------
// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
// ------------------------------------------------------------------------
// Step 1. Find the parent  directory
// Step 2. Find the current directory
// Step 3. Print the path
// ------------------------------------------------------------------------
int
FS::pwd()
{
    // std::cout << "FS::pwd()... Printing working directory \n";

    // check if the current directory is the root directory
    if (current_directory == ROOT_BLOCK) {
        std::cout << "/\n";
        return 0;
    }

    std::vector<std::string> path;                      // create a vector to store the path

    uint16_t current_dir = current_directory;            // set the current directory
    while (current_dir != ROOT_BLOCK) {                 // iterate over the directories
        uint8_t dir_blk[BLOCK_SIZE];
        if (disk.read(current_dir, dir_blk) != 0) {      // read the directory
            std::cerr << "FS::pwd()... Error reading directory from disk\n";
            return -1;
        }

        dir_entry* dir_entries = reinterpret_cast<dir_entry*>(dir_blk); // cast the block to dir_entry

        // find the parent directory
        uint16_t parent_dir = dir_entries[0].first_blk;  // get the parent directory


        // find the name of the current directory
        uint8_t parent_blk[BLOCK_SIZE];
        if (disk.read(parent_dir, parent_blk) != 0) {    // read the parent directory
            std::cerr << "FS::pwd()... Error reading parent directory from disk " << parent_dir << "\n";
            return -1;
        }

        dir_entry* parent_entries = reinterpret_cast<dir_entry*>(parent_blk); // cast the block to dir_entry

        std::string current_dir_name;                                   // create a string to store the current directory name
        for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {      // iterate over the directory entries
            if (parent_entries[i].first_blk == current_dir) {           // check if the first block is the current directory
                current_dir_name = parent_entries[i].file_name;         // get the current directory name
                break;
            }
        }

        path.push_back(current_dir_name);                               // add the current directory name to the path

        current_dir = parent_dir;                                       // set the current directory
    }

    // print the path
    std::reverse(path.begin(), path.end());                            // reverse the path
    std::string full_path = "/";                                        // create a string to store the full path
    for (const auto& dir : path) {                                     // iterate over the path
        full_path += dir + "/";                                        // add the directory to the full path
    }

    // remove the last '/'
    full_path.pop_back();                                              // remove the last '/'
    std::cout << full_path << "\n";                                    // print the full path
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}

// Helper function to read the directory
dir_entry* FS::read_directory(uint8_t* dir_blk)
{
    if (disk.read(current_directory, dir_blk) != 0)                                                // read the root directory
    {
        std::cerr << "FS::read_directory()... Error reading root directory from disk\n";
        return nullptr;
    }
    return reinterpret_cast<dir_entry*>(dir_blk);                                           // cast the block to dir_entry
}

// Helper function to write the FAT to the disk
int FS::write_fat_to_disk()
{
    uint8_t blk[BLOCK_SIZE];
    std::memcpy(blk, fat, BLOCK_SIZE); // copy the FAT to the block
    if (disk.write(FAT_BLOCK, blk) != 0) {
        std::cerr << "FS::write_fat_to_disk()... Error writing FAT to disk\n";
        return -1;
    }
    return 0;
}

// Helper function to find a file in the directory
int FS::find_file(const std::string &filepath, dir_entry &file_entry)
{
    // read the root directory
    uint8_t dir_blk[BLOCK_SIZE];
    dir_entry* dir_entries = read_directory(dir_blk);
    if (!dir_entries) {
        return -1;                         // cast the block to dir_entry
    }

    // find the file in the directory
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {                              // iterate over the directory entries
        if (std::strcmp(dir_entries[i].file_name, filepath.c_str()) == 0) {                 // check if the file exists
            file_entry = dir_entries[i];                                                    // get the file entry
            return i;                                                                       // return the index
        }
    }
    return -1; // return -1 if the file is not found
}

// Helper function to allocate a free block
int FS::allocate_block()
{
    for (int i = 2; i < disk.get_no_blocks(); ++i) {    // iterate over the blocks
        if (fat[i] == FAT_FREE) {                       // check if the block is free
            fat[i] = FAT_EOF;                           // mark the block as the last block
            return i;                                   // return the block index
        }
    }
    return -1;                                          // return -1 if no block is found
}

// Helper function to copy the file content block-by-block
bool FS::copy_file(int source_block, int dest_block)
{
    uint8_t blk[BLOCK_SIZE];
    while (source_block != FAT_EOF) {                   // iterate over the blocks
        if (disk.read(source_block, blk) != 0) {         // read the block from the disk
            return false;                               // return false if there is an error
        }

        if (disk.write(dest_block, blk) != 0) {          // write the block to the disk
            return false;                               // return false if there is an error
        }

        source_block = fat[source_block];                // get the next block

        if (source_block != FAT_EOF) {                   // check if the block is not the last block
            int next_block = allocate_block();           // allocate a new block
            if (next_block == -1) {                      // check if there is enough space on the disk
                return false;                           // return false if there is an error
            }
            fat[dest_block] = next_block;                // update the FAT
            dest_block = next_block;                     // set the destination block
        }
    }
    return true;                                        // return true if the file is copied successfully
}   

// helper function to marks all blocks used by a file as free in the FAT
void FS::free_blocks(int first_block)
{
    int current_block = first_block;
    while (current_block != FAT_EOF) {                   // iterate over the blocks
        int next_block = fat[current_block];             // get the next block
        fat[current_block] = FAT_FREE;                   // mark the block as free
        current_block = next_block;                      // set the current block
    }
}

// helper function to find the last block of a file
int FS::find_last_block(int first_block)
{
    if (first_block == FAT_EOF) {                         // check if the file is empty
        return -1;                                        // return -1 if the file is empty
    }

    int current_block = first_block;
    while (fat[current_block] != FAT_EOF) {              // iterate over the blocks
        if (current_block < 0) return -1;               // Prevent accessing invalid blocks
        current_block = fat[current_block];              // get the next block
    }
    return current_block;                                // return the last block
}

// helper function to find an entry in the directory by name
int FS::find_entry_by_name(dir_entry* entries, const std::string& name)

{
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {  // iterate over the directory entries
        if (std::strcmp(entries[i].file_name, name.c_str()) == 0) { // check if the file exists
            return i;                                             // return the index
        }
    }
    return -1; // return -1 if the file is not found
}

// helper function to handle cp into a directory (both cp source and dest dir given)
int FS::cp_into_dir(dir_entry src_entry, uint16_t dest_dir_block, std::string new_filename) {
    uint8_t dest_dir_blk[BLOCK_SIZE];
    if (disk.read(dest_dir_block, dest_dir_blk) != 0) {
        return -1;
    }

    dir_entry* dest_dir_entries = reinterpret_cast<dir_entry*>(dest_dir_blk);                   // cast the block to dir_entry

    int free_index = find_empty_slot(dest_dir_entries);                                        // find an empty slot in the directory

    if (free_index == -1) {
        return -1;
    }

    dir_entry new_entry = src_entry;                                                          // copy the source entry
    if (new_filename != "") {
        strncpy(new_entry.file_name, new_filename.c_str(), sizeof(new_entry.file_name) - 1);  // set the new filename
    }

    if (copy_file_content(src_entry.first_blk, new_entry.first_blk, new_entry.size) != 0) {                   // copy the file contents
        return -1;
    }

    dest_dir_entries[free_index] = new_entry;                                                // add the new entry to the directory

    if (disk.write(dest_dir_block, dest_dir_blk) != 0) {                                      // write the updated directory to the disk
        return -1;
    }

    write_fat_to_disk();                                                                     // update the FAT on the disk
    return 0;
}

// helper function to find an empty slot in the directory
int FS::find_empty_slot(dir_entry* entries) {
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry); ++i) {  // iterate over the directory entries
        if (entries[i].file_name[0] == '\0') {                   // check if the entry is empty
            return i;                                           // return the index
        }
    }
    return -1; // return -1 if the slot is not found
}

// helper function to copy the file contents
int FS::copy_file_content(uint16_t src_first_blk, uint16_t& dest_first_blk, uint32_t& file_size) {
    uint8_t block[BLOCK_SIZE];
    int src_block = src_first_blk;
    int dest_block = allocate_block();

    if (dest_block == -1) {
        std::cerr << "FS::cp()... No space on disk\n";
        return -1;
    }

    dest_first_blk = dest_block;                                    // set the first block of the destination file
    int prev_dest_block = dest_block;                               // set the previous destination block

    while (src_block != FAT_EOF) {
        if (disk.read(src_block, block) != 0) {                     // read the source block
            std::cerr << "FS::cp()... Error reading source block\n";
            return -1;
        }

        if (disk.write(dest_block, block) != 0) {                   // write the block to the disk
            std::cerr << "FS::cp()... Error writing to destination block\n";
            return -1;
        }

        src_block = fat[src_block];                                 // get the next source block

        if (src_block != FAT_EOF) {                                 // check if the block is not the last block
            int next_block = allocate_block();                      // allocate a new block
            if (next_block == -1) {                                 // check if there is enough space on the disk
                std::cerr << "FS::cp()... No space on disk during copy\n";
                return -1;
            }

            fat[prev_dest_block] = next_block;                      // update the FAT
            prev_dest_block = next_block;                           // set the previous destination block
        }
    }

    fat[prev_dest_block] = FAT_EOF;                                 // mark the last block as the last block
    return 0;
}