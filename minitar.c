#include <fcntl.h>
#include <grp.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <unistd.h>

#include "minitar.h"
#include <stdlib.h>
#include <errno.h>
#define NUM_TRAILING_BLOCKS 2
#define MAX_MSG_LEN 512

/*
 * Helper function to compute the checksum of a tar header block
 * Performs a simple sum over all bytes in the header in accordance with POSIX
 * standard for tar file structure.
 */
void compute_checksum(tar_header *header) {
    // Have to initially set header's checksum to "all blanks"
    memset(header->chksum, ' ', 8);
    unsigned sum = 0;
    char *bytes = (char *)header;
    for (int i = 0; i < sizeof(tar_header); i++) {
        sum += bytes[i];
    }
    snprintf(header->chksum, 8, "%07o", sum);
}

/*
 * Populates a tar header block pointed to by 'header' with metadata about
 * the file identified by 'file_name'.
 * Returns 0 on success or -1 if an error occurs
 */
int fill_tar_header(tar_header *header, const char *file_name) {
    memset(header, 0, sizeof(tar_header));
    char err_msg[MAX_MSG_LEN];
    struct stat stat_buf;
    // stat is a system call to inspect file metadata
    if (stat(file_name, &stat_buf) != 0) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to stat file %s", file_name);
        perror(err_msg);
        return -1;
    }

    strncpy(header->name, file_name, 100); // Name of the file, null-terminated string
    snprintf(header->mode, 8, "%07o", stat_buf.st_mode & 07777); // Permissions for file, 0-padded octal

    snprintf(header->uid, 8, "%07o", stat_buf.st_uid); // Owner ID of the file, 0-padded octal
    struct passwd *pwd = getpwuid(stat_buf.st_uid); // Look up name corresponding to owner ID
    if (pwd == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up owner name of file %s", file_name);
        perror(err_msg);
        return -1;
    }
    strncpy(header->uname, pwd->pw_name, 32); // Owner  name of the file, null-terminated string

    snprintf(header->gid, 8, "%07o", stat_buf.st_gid); // Group ID of the file, 0-padded octal
    struct group *grp = getgrgid(stat_buf.st_gid); // Look up name corresponding to group ID
    if (grp == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up group name of file %s", file_name);
        perror(err_msg);
        return -1;
    }
    strncpy(header->gname, grp->gr_name, 32); // Group name of the file, null-terminated string

    snprintf(header->size, 12, "%011o", (unsigned)stat_buf.st_size); // File size, 0-padded octal
    snprintf(header->mtime, 12, "%011o", (unsigned)stat_buf.st_mtime); // Modification time, 0-padded octal
    header->typeflag = REGTYPE; // File type, always regular file in this project
    strncpy(header->magic, MAGIC, 6); // Special, standardized sequence of bytes
    memcpy(header->version, "00", 2); // A bit weird, sidesteps null termination
    snprintf(header->devmajor, 8, "%07o", major(stat_buf.st_dev)); // Major device number, 0-padded octal
    snprintf(header->devminor, 8, "%07o", minor(stat_buf.st_dev)); // Minor device number, 0-padded octal

    compute_checksum(header);
    return 0;
}

/*
 * Removes 'nbytes' bytes from the file identified by 'file_name'
 * Returns 0 upon success, -1 upon error
 * Note: This function uses lower-level I/O syscalls (not stdio), which we'll learn about later
 */
int remove_trailing_bytes(const char *file_name, size_t nbytes) {
    char err_msg[MAX_MSG_LEN];
    // Note: ftruncate does not work with O_APPEND
    int fd = open(file_name, O_WRONLY);
    if (fd == -1) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to open file %s", file_name);
        perror(err_msg);
        return -1;
    }
    //  Seek to end of file - nbytes
    off_t current_pos = lseek(fd, -1 * nbytes, SEEK_END);
    if (current_pos == -1) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to seek in file %s", file_name);
        perror(err_msg);
        close(fd);
        return -1;
    }
    // Remove all contents of file past current position
    if (ftruncate(fd, current_pos) == -1) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to truncate file %s", file_name);
        perror(err_msg);
        close(fd);
        return -1;
    }
    if (close(fd) == -1) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to close file %s", file_name);
        perror(err_msg);
        return -1;
    }
    return 0;
}

//FERROR USE IMPORTANT
int create_archive(const char *archive_name, const file_list_t *files) {
    FILE *fp = fopen(archive_name, "w");
    char err_msg[MAX_MSG_LEN];
    //errcheck #1, check if the archive can be open
    if(fp==NULL){
        snprintf(err_msg, MAX_MSG_LEN, "Failed to open archive file %s", archive_name);
        perror(err_msg);
        //You shouldn't close a NULL pointer, results in segfault
        return -1;
    }
    tar_header current_header;
    node_t *current_node = files->head;
    FILE *fnode;
    while(current_node!=NULL){
        //terminates at end of list, when current node =NULL
        //This will not run in the event where &files is empty(Creates a 1024 byte footer and thats it) (Minitar.h says we can ignore this case anyway.)
        if(fill_tar_header(&current_header, current_node->name) == -1){
            //if filltar header fails...
            snprintf(err_msg, MAX_MSG_LEN, "Function fill_tar_header failed on filename %s", current_node->name);
            perror(err_msg);
            fclose(fp);
            return -1;
        }
        if (fwrite(&current_header, 1, 512, fp)!=512){
            //if fwrite fails.
            snprintf(err_msg, MAX_MSG_LEN, "Failed to write %s header to archive", current_header.name);
            perror("Error: failed to write header to archive %s");
            fclose(fp);
            return -1;
        }
        fnode = fopen(current_node->name, "r");
        //Fnode represents the file we are reading from or copying to the archive.
        if(fnode ==NULL){
            snprintf(err_msg, MAX_MSG_LEN, "Failed to open file %s", current_node->name);
            perror(err_msg);
            fclose(fp);
            //Shouldn't close NULL file pointers
            return -1;
        }
        else{
        char buffer[512];
        memset(buffer, 0, 512);
        while(fread(buffer, sizeof(char),512, fnode)>0){ //There is no need to check length of read here, as buffer is always filled with 0's, which must be added to the end of the file in the archive
        //Therefore no need to error check this fread. 
            if(fwrite(buffer, sizeof(char), 512, fp)!=512){
                snprintf(err_msg, MAX_MSG_LEN, "Failed to write file %s to archive", current_node->name);
                perror(err_msg);
                fclose(fp);
                fclose(fnode);
                return -1;
            }
            //reset buffer to 0's (This will always set)
            memset(buffer, 0, 512);
        }
        }
        current_node = current_node->next;
        fclose(fnode);
    }
    char zero[1024];
    memset(zero, 0, 1024);
    //this is to set up the footer, where the last 1024 bytes are simply made to be 0. 
    if(fwrite(zero, sizeof(char), 1024, fp)!=1024){
        snprintf(err_msg, MAX_MSG_LEN, "fwrite failed to append 1024 0's at the end of archive %s", archive_name);
        perror(err_msg);
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}
//I was told I never need to error check fclose

int append_files_to_archive(const char *archive_name, const file_list_t *files) {
    char err_msg[MAX_MSG_LEN];
    FILE *checkexistence = fopen(archive_name, "r");
    //the reason I make a whole new file is to make certain that the file exists prior to the read and write functions.
    //I assume that this would not work when opening an read/write file so I did an extra step.
    if(checkexistence==NULL){
        snprintf(err_msg, MAX_MSG_LEN, "Archive %s Does not exist, and cannot be appended", archive_name);
        perror(err_msg);
        return -1;
    }
    fclose(checkexistence);
    FILE *fp = fopen(archive_name, "r+");
    if(fp==NULL){
        snprintf(err_msg, MAX_MSG_LEN, "Failed to open archive file %s", archive_name);
        //Can't close a null file pointer results in error
        perror(err_msg);
        return -1;
    }
    if(fseek(fp, -1024, SEEK_END)!=0){
        snprintf(err_msg, MAX_MSG_LEN, "Failed to Fseek to start of archive footer %s", archive_name);
        perror(err_msg);
        fclose(fp);
        return -1;
    }
    tar_header current_header;
    node_t *current_node = files->head;
    FILE *fnode;
    while(current_node!=NULL){
        if(fill_tar_header(&current_header, current_node->name) == -1){
            snprintf(err_msg, MAX_MSG_LEN, "Failed to populate header %s", current_node->name);
            perror(err_msg);
            return -1;
        }
        if(fwrite(&current_header, 1, 512, fp)!=512){
            snprintf(err_msg, MAX_MSG_LEN, "invalid header length, couldn't write header to archive %s", current_node->name);
            perror(err_msg);
            fclose(fp);
            return -1;
        }
        fnode = fopen(current_node->name, "r");
        if(fnode==NULL){
            snprintf(err_msg, MAX_MSG_LEN, "Failed to open file %s", current_node->name);
            perror(err_msg);
            fclose(fp);
            //Can't close a null file pointer results in error (segfault)
            return -1;
        }
        char buffer[512];
        memset(buffer, 0, 512);
        while(fread(buffer, sizeof(char),512, fnode)>0){
        //There is no need to check length of read here, as buffer is always filled with 0's, which must be added to the end of the file in the archive
        //Therefore no need to error check this fread. (It will always be 512) Confirmed by TA
            if(fwrite(buffer, sizeof(char), 512, fp)!=512){
                snprintf(err_msg, MAX_MSG_LEN, "Failed to write file %s to archive ", current_node->name);
                perror(err_msg);
                fclose(fp);
                fclose(fnode);
                return -1;
            }
            memset(buffer, 0, 512);
            //reset buffer to all 0's
        }
        current_node = current_node->next;
        fclose(fnode);
    }
    char zero[1024];
    //footer all bytes are 0, and written to archive
    memset(zero, 0, 1024);
    if(fwrite(zero, sizeof(char), 1024, fp)!=1024){
        snprintf(err_msg, MAX_MSG_LEN, "fwrite failed to append 1024 0's at the end of archive %s", archive_name);
        perror(err_msg);
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
    //complete
}

int get_archive_file_list(const char *archive_name, file_list_t *files) {
    FILE  *fp = fopen(archive_name, "r");
    char err_msg[MAX_MSG_LEN];
    if(fp==NULL){
        snprintf(err_msg, MAX_MSG_LEN, "Failed to open archive file %s", archive_name);
        //Can't close a null file pointer results in error
        perror(err_msg);
        return -1;
    }
    if(fseek(fp, -512, SEEK_END)!=0){
        snprintf(err_msg, MAX_MSG_LEN, "Fseek() failed to reach end of archive %s", archive_name);
        perror(err_msg);
        fclose(fp);
        return -1;
    }
    long int sz = ftell(fp);
    //this will give us the end condition, when ftell is 512 bytes into the footer
    if(sz==-1){
        snprintf(err_msg, MAX_MSG_LEN, "Ftell() failed to reach end of archive %s", archive_name);
        perror(err_msg);
        fclose(fp);
        return -1;
    }
    //reset fp to start
    if(fseek(fp, 0, SEEK_SET)!=0){
        perror("fseek failed to reset to start of archive");
        fclose(fp);
        return -1;
        }
    tar_header current_header;
    if(fread(&current_header, 1,512,fp)!=512){
        perror("Failed to fill current header from archive");
        fclose(fp);
        return -1;
    }
    //sets up and reads the first header in.
    int x = 0;
    long int currenttell = ftell(fp);
    if(currenttell==-1){
        snprintf(err_msg, MAX_MSG_LEN, "Ftell() failed to reach the start of the archive %s", archive_name);
        perror(err_msg);
        fclose(fp);
        return -1;
    }
    //I tried to do a strcmp to see if it was 1024 bits of 0's but it didn't work, so I ended up doing it based on the size
    //I used -512, because the loop would read in 
    while(currenttell!=sz){
        //originally, I had ftell(fp) instead of currenttell, but I was told by a TA to do it this way for error checking
        if(file_list_add(files, current_header.name)==1){
            snprintf(err_msg, MAX_MSG_LEN, "File list add failed at %s", current_header.name);
            perror(err_msg);
            fclose(fp);
        }
        //Realized that size was not the 512 multiple it needed to be, this will always round up from size to nearest multiple of 512
        //THis is converting to base 8, adding 511 (to round up) integer dividing by 512 and multiplying by 512 to give me the number of bits to offset the seek.
        x=512*((strtol(current_header.size,NULL, 8)+511)/512);
        //I chose this method over scanf as it makes more sense to me...
        if(fseek(fp, x,SEEK_CUR)!=0){
            perror("fseek failed");
            fclose(fp);
            return -1;
        }
        if(fread(&current_header, 1,512,fp)!=512){
            perror("Failed to Populate header");
            fclose(fp);
            return -1;
        }
        currenttell =ftell(fp);
        if(currenttell==-1){
            snprintf(err_msg, MAX_MSG_LEN, "Ftell() failed to reach calculate position in the archive %s", archive_name);
            perror(err_msg);
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return 0;
    //completed
}

int extract_files_from_archive(const char *archive_name) {
    int trailingbytes;
    char err_msg[MAX_MSG_LEN];
    FILE  *fp = fopen(archive_name, "r"); 
    FILE *fdest;
    if(fp==NULL){
        snprintf(err_msg, MAX_MSG_LEN, "Failed to open archive file %s", archive_name);
        perror(err_msg);
        //Can't close a null file pointer results in error
        return -1;
    }
    //I probably do not need to error check every fseek but I figured better safe than sorry
    if(fseek(fp, -512, SEEK_END)!=0){
        perror("fseek() failed");
            fclose(fp);
        return -1;
    }
    long int sz = ftell(fp);
    //this will give us the end condition, when ftell is 512 bytes into the footer
    if(sz==-1){
        snprintf(err_msg, MAX_MSG_LEN, "Ftell() failed to reach end of archive %s", archive_name);
        perror(err_msg);
        fclose(fp);
        return -1;
    }
    int x = 0;
    char buffer[512];
    //I probably do not need to error check every fseek but I figured better safe than sorry
    if(fseek(fp, 0, SEEK_SET)!=0){
        perror("fseek() failed");
        fclose(fp);
        return -1;
    }
    
    tar_header current_header;
    if(fread(&current_header, 1,512,fp)!=512){
        perror("failed to read in header");
        fclose(fp);
        return -1;
    }
    long int currenttell = ftell(fp);
    if(currenttell==-1){
        snprintf(err_msg, MAX_MSG_LEN, "Ftell() failed to reach the start of the archive %s", archive_name);
        perror(err_msg);
        fclose(fp);
        return -1;
    }
    while(currenttell<sz){
        //Only the most recent updated file is extracted, as an earlier one will simply be overwritten by fopen(fileofsamename, "w")
        fdest = fopen(current_header.name, "w");
        if (fdest == NULL) {
            snprintf(err_msg, MAX_MSG_LEN, "Failed to open file %s", current_header.name);
            perror(err_msg);
            //Can't close a null file pointer results in error
            fclose(fp);
            return -1;
        }
        else{
        //Again, I chose strtol instead of fscanf because it made more sense to me.
        x=((strtol(current_header.size,NULL, 8)+511)/512);
        for(int k=0; k<x; k+=1){
            memset(buffer, 0, 512);
            if(fread(buffer, 1, 512, fp)!=512){
                snprintf(err_msg, MAX_MSG_LEN, "Failed to read %s from archive", current_header.name);
                perror(err_msg);
                fclose(fp);
                fclose(fdest);
                return -1;
            }
            if(fwrite(buffer, 1,512, fdest)!=512){
                snprintf(err_msg, MAX_MSG_LEN, "Failed to write %s to file from archive", current_header.name);
                perror(err_msg);
                fclose(fp);
                fclose(fdest);
                return -1;
            }
        }
        fclose(fdest);
        //add in all data from archive, then remove trailing zeroes as needed. 
        trailingbytes = 512*x -(strtol(current_header.size,NULL, 8));
        //trailingbytes is the number of trailing0's from file (That was added by the archive, normal trailing 0's can still exist if they were present in the original file)
        if(remove_trailing_bytes(current_header.name, trailingbytes)==-1){
            snprintf(err_msg, MAX_MSG_LEN, "Failed to remove trailing bytes of %s", current_header.name);
            perror(err_msg);
            fclose(fp);
            return -1;
        }
        //remove trailing bytes has its own error checking so I don't use errval
        if(fread(&current_header, 1,512,fp)!=512){
            perror("Failed to fill current header from archive");
            fclose(fp);
            return -1;
        }
        currenttell =ftell(fp);
        if(currenttell==-1){
            snprintf(err_msg, MAX_MSG_LEN, "Ftell() failed to reach calculate position in the archive %s", archive_name);
            perror(err_msg);
            fclose(fp);
            return -1;
        }
        }
    }
    fclose(fp);
    return 0;
}

// Should I error check fseek? fread? fwrite?
//Every fseek every fread and fwrite
//compress and rename to test
//print error for all given functions

//WHAT ABOUT MEMSET

//REMEMBEr FTELL