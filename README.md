## Overview

This project implements a simple filesystem using a simulated disk. It is part of Lab 3 of the Operating Systems course at Blekinge Institute of Technology. The goal is to explore filesystem concepts and provide hands-on experience with basic file management operations.

---

## Features

The implemented filesystem supports the following operations:

1. **Formatting**: Initialize or reset the filesystem.
2. **File Operations**:
   - Create files.
   - Read file contents (`cat`).
   - List files and directories (`ls`).
   - Copy and move files (`cp`, `mv`).
   - Append content from one file to another.
   - Remove files.
3. **Directory Operations**:
   - Create directories (`mkdir`).
   - Navigate directories (`cd`, `pwd`).
4. **Access Rights**:
   - Modify access rights (`chmod`).

---

## Directory Structure

The project contains the following files:

### Header Files
- `fs.h`: Filesystem class definition.
- `shell.h`: Shell interface for interacting with the filesystem.
- `disk.h`: Simulated disk interface.
- `direntry.h`: Defines the structure of a directory entry.

### Source Files
- `fs.cpp`: Implementation of the filesystem methods.
- `shell.cpp`: Shell implementation to handle user commands.
- `disk.cpp`: Simulated disk read/write operations.
- `main.cpp`: Entry point of the program.

### Test Scripts
- `test_script1.cpp`: Tests formatting and file creation.
- `test_script2.cpp`: Tests file copy, move, and remove operations.
- `test_script3.cpp`: Tests directory creation and navigation.
- `test_script4.cpp`: Tests advanced directory and file operations.
- `test_script5.cpp`: Tests file permissions and appending files.

### Input Files
- `input1.txt`: Sample file content for testing.
- `input2.txt`: Additional sample content.
- `input3.txt`: Large file content for boundary testing.

---

## Getting Started

### Prerequisites

- **C++ Compiler**: Ensure you have GCC or any C++17-compatible compiler installed.
- **Make**: Use the provided `Makefile` for building the project.

### Compilation

To compile the project:
```bash
make
```

### Running the Shell

Execute the program:
```bash
./filesystem
```

You will enter an interactive shell to perform filesystem operations.

---

## Commands

| Command           | Description                                      |
|-------------------|--------------------------------------------------|
| `format`          | Format the filesystem.                          |
| `create <file>`   | Create a new file and input its contents.        |
| `cat <file>`      | Display the contents of a file.                 |
| `ls`              | List all files and directories in the current directory. |
| `cp <src> <dest>` | Copy a file to a new location.                  |
| `mv <src> <dest>` | Move or rename a file.                          |
| `rm <file>`       | Delete a file.                                  |
| `append <src> <dest>` | Append contents of one file to another.    |
| `mkdir <dir>`     | Create a new directory.                         |
| `cd <dir>`        | Change the current directory.                   |
| `pwd`             | Print the current directory path.               |
| `chmod <rights> <file>` | Change file access rights.                |
| `help`            | Display a list of available commands.           |
| `quit`            | Exit the shell.                                 |

---

## Testing

### Example
```bash
./filesystem < test_commands.txt
```

This will execute the commands specified in the `test_commands.txt` file.

---
