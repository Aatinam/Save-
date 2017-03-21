CSC-209
Assignment 3 




A hard-link is a pointer to a file’s inode, we have implemented a design where copy_ftree() is called recursively and each child subdirectory in the source is handed off to a new process by calling fork(), the problem this poses is that separate processes don’t share memory which means that if after calling fork, two different processes encounter a hard-link of the same file both would copy the entire file to the destination. A way around this would be to use pipes to establish communication between processes, one possible implementation could be that the child processes computes hash of the files to be copied and the parent then writes files to the destination, linking files with the same hash using link().