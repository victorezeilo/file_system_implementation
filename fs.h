#include <iostream>
#include <cstdint>
#include "disk.h"

#ifndef __FS_H__
#define __FS_H__

#define ROOT_BLOCK 0
#define FAT_BLOCK 1
#define FAT_FREE 0
#define FAT_EOF -1

#define TYPE_FILE 0
#define TYPE_DIR 1
#define READ 0x04
#define WRITE 0x02
#define EXECUTE 0x01

struct dir_entry {
    char file_name[56]; // name of the file / sub-directory
    uint32_t size; // size of the file in bytes
    uint16_t first_blk; // index in the FAT for the first block of the file
    uint8_t type; // directory (1) or file (0)
    uint8_t access_rights; // read (0x04), write (0x02), execute (0x01)
};

class FS {
private:
    Disk disk;
    // size of a FAT entry is 2 bytes
    int16_t fat[BLOCK_SIZE/2];
    // helper function to read the directory
    dir_entry* read_directory(uint8_t* dir_blk);
    // helper function to write the directory
    int write_fat_to_disk();
    // Helper function to find an entry in the directory by name
    int find_entry_by_name(dir_entry* entries, const std::string &name);
    // Helper function to find an empty slot in the directory
    int find_empty_slot(dir_entry* entries);
    // Helper function to copy a directory entry into a destination directory block
    int copy_entry_to_dir_block(dir_entry* source, uint8_t* dest_dir_blk);
    // Helper function to copy file content from source to destination (used by cp)
    int copy_file_content(uint16_t source_first_block, uint16_t& dest_first_block, uint32_t& file_size);
    // helper function to handle cp into a directory (both cp source and dest dir given)
    int cp_into_dir(dir_entry source_entry, uint16_t dest_dir_block, std::string new_file_name = "");
    // Helper function to resolve a path to a directory block
    std::pair<int, std::string> resolve_path(const std::string& path, bool is_dir = true, bool return_parent = false);
    uint16_t current_directory;



public:
    FS();
    ~FS();
    // formats the disk, i.e., creates an empty file system
    int format();
    // create <filepath> creates a new file on the disk, the data content is
    // written on the following rows (ended with an empty row)
    int create(std::string filepath);
    // cat <filepath> reads the content of a file and prints it on the screen
    int cat(std::string filepath);
    // ls lists the content in the current directory (files and sub-directories)
    int ls();

    // cp <sourcepath> <destpath> makes an exact copy of the file
    // <sourcepath> to a new file <destpath>
    int cp(std::string sourcepath, std::string destpath);
    // mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
    // or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
    int mv(std::string sourcepath, std::string destpath);
    // rm <filepath> removes / deletes the file <filepath>
    int rm(std::string filepath);
    // append <filepath1> <filepath2> appends the contents of file <filepath1> to
    // the end of file <filepath2>. The file <filepath1> is unchanged.
    int append(std::string filepath1, std::string filepath2);

    // mkdir <dirpath> creates a new sub-directory with the name <dirpath>
    // in the current directory
    int mkdir(std::string dirpath);
    // cd <dirpath> changes the current (working) directory to the directory named <dirpath>
    int cd(std::string dirpath);
    // pwd prints the full path, i.e., from the root directory, to the current
    // directory, including the current directory name
    int pwd();

    // chmod <accessrights> <filepath> changes the access rights for the
    // file <filepath> to <accessrights>.
    int chmod(std::string accessrights, std::string filepath);

    // Helper function to find a file in the directory
    int find_file(const std::string &filepath, dir_entry &file_entry);
    // Helper function to allocate a free block
    int allocate_block();
    // Helper function to copy the file content block-by-block
    bool copy_file(int source_block, int dest_block);
    // Helper function to marks all blocks used by a file as free in the FAT
    void free_blocks(int first_block);
    // Helper function to find the last block of a file
    int find_last_block(int first_block);
};

#endif // __FS_H__
