## Operating Systems and Networks | Monsoon 2023
### Course Project

Team 31: Revanth Gundam, Nanda Rajiv, Tanveer Ul Mustafa

# Implementing a Network File System

Instructions to run the code:
- Enter the `working/` directory
- Run `make all` to compile the three files `ser.c`, `cli.c` and `ss.c`
- First run the naming server by running `./s`
- Run a storage server by running `./ss` and initialise it by entering ports and entering accessible paths
- Run a second storage server (we require a minimum of two to be runnning) by repeating the above process
- Check that the terminal where the Naming Server is running is listening for clients 
- Run a client by running `./c` and begin to enter your commands

The commands that can be run are:
- `create <filepath>`: Creates a file or a folder with the given name at the path specified
- `read <filepath>`: Reads the contents of the file at the path specified
- `write <filepath>`: Initiates a back-and-forth dialogue on the terminal for the user to input what they want to write into the file at `filepath` and writes the data to the file
- `delete <filepath>`: Deletes the file or folder (recursively) at the path specified
- `get_info <filepath>`: Returns the size and permissions of the file at the path specified
- `copy <file/folder>`: Initiates a dialogue where the user can enter the source and destination path of the file or folder to be copied. The file or folder is recursively copied to the destination path

Features:
- The naming server is able to handle multiple clients at the same time
- The naming server is able to handle multiple storage servers at the same time
- Storage servers can be added and removed dynamically

Assumptions:
- The naming server is <b>always running</b>. It is the first thing that is initialised, and the last thing to be exited.
- No two storage servers have the same accessible paths. Effectively, we want the whole system to behave as one single file system that is distributed across multiple storage servers. So, we assume that the paths that are accessible to each storage server are <b>unique</b>. So that means there is only one path `./` which is accessible on one storage server. There may be a `./folder1` which happens to be on another storage server. No two storage servers have the same accessible paths.
- No input of filepaths will end in a `/`. So, `./folder1` is a valid filepath for input, but `./folder1/` is not.