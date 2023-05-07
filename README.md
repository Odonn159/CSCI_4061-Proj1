# CSCI_4061-Proj1
The goal of this project is to develop a simplified version of the tar utility, called minitar. tar is one of the popular tools among users of Linux and other Unix-derived operating systems. It creates a single archive file from several member files. Thus, tar is very similar to the zip tool we use for code submission in this class, although it does not perform file compression as zip does.  

tar files are made up of a sequence of 512-byte blocks. Each member file in the archive is represented by a sub-sequence of blocks within the file. The first block in the subsequence is a header containing metadata about the file, such as its name and its size in bytes. The contents of the member file are then reproduced, completely unaltered from the original file, in the subsequent blocks of the archive file.  


Any command-line invocation of minitar will adhere to the following pattern:  


> ./minitar <"operation"> -f <archive_name> <file_name_1> <file_name_2> ... <file_name_n>   
> 
<"operation"> may be any one of the following:  


-c: Create a new archive file with the name <archive_name> and including all member files identified by each <file_name_i> command-line argument.  

-a: Append more member files identified by each <file_name_i> argument to the existing archive file identified by <archive_name>.   

-t: List out (print to the terminal) the name of each member file included in the archive identified by <archive_name> (no <file_name_i> arguments are necessary).   

-u: Update all member files identified by the <file_name_i> arguments contained in the archive file identified by <archive_name>. The archive must already contain all of these files, and new versions of each file will be appended to the end of the archive.   

-x: Extract all member files from the archive identified by the <archive_name> argument and save them as regular files in the current working directory. No <file_name_i> arguments are necessary.   

