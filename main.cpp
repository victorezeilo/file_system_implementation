#include "shell.h"
#include "fs.h"
#include "disk.h"

int
main(int argc, char **argv)
{
    Shell shell;
    shell.run();


    // FS fs;
    // // format the disk first
    // fs.format();

    // // create test directory
    // std::cout << "Creating directory: dir1, subdir2...\n";
    // fs.mkdir("dir1");
    // fs.mkdir("subdir2");


    // // list directory content
    // std::cout << "Listing directory content...\n";
    // fs.ls();


    // // Create directories and navigate
    // fs.mkdir("d1");
    // fs.cd("d1");

    // fs.mkdir("subd1");
    // fs.cd("subd1");

    // // Print current path
    // fs.pwd(); // Should print /d1/subd1

    // // Go back to root
    // fs.cd("..");
    // fs.cd("..");
    // fs.pwd(); // Should print /

    return 0;
}
