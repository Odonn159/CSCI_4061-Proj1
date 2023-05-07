#ifndef _MINITAR_H
#define _MINITAR_H
#include "file_list.h"

#define BLOCK_SIZE 512

// Standard tar header layout defined by POSIX
typedef struct {
    // File's name, as a null-terminated string
    char name[100];
    // File's permission bits
    char mode[8];
    // Numerical ID of file's owner, 0-padded octal
    char uid[8];
    // Numerical ID of file's group, 0-padded octal
    char gid[8];
    // Size of file in bytes, 0-padded octal
    char size[12];
    // Modification time of file in Unix epoch time, 0-padded octal
    char mtime[12];
    // Checksum (simple sum) header bytes, 0-padded octal
    char chksum[8];
    // File type (use constants defined below)
    char typeflag;
    // Unused for this project
    char linkname[100];
    // Indicates which tar standard we are using
    char magic[6];
    char version[2];
    // Name of file's user, as a null-terminated string
    char uname[32];
    // Name of file's group, as a null-terminated string
    char gname[32];
    // Major device number, 0-padded octal
    char devmajor[8];
    // Minor device number, 0-padded octal
    char devminor[8];
    // String to prepend to file name above, if name is longer than 100 bytes
    char prefix[155];
    // Padding to bring total struct size up to 512 bytes
    char padding[12];
} tar_header;

// Constants for tar compatibility information
#define MAGIC "ustar"

// Constants to represent different file types
// We'll only use regular files in this project
#define REGTYPE '0'
#define DIRTYPE '5'

/*
 * Create a new archive file with the name 'archive_name'.
 * The archive should contain all files contained in the 'files' list.
 * You can assume in this project that at least one member file is specified.
 * You may also assume that all the elements of 'files' exist.
 * If an archive of the specified name already exists, you should overwrite it
 * with the result of this operation.
 * This function should return 0 upon success or -1 if an error occurred
 */
int create_archive(const char *archive_name, const file_list_t *files);

/*
 * Append each file specified in 'files' to the archive with the name 'archive_name'.
 * You can assume in this project that at least one new file to append is specified.
 * You may also assume that all files to be appended exist.
 * This function should return 0 upon success or -1 if an error occurred.
 */
int append_files_to_archive(const char *archive_name, const file_list_t *files);

/*
 * Add the name of each file contained in the archive identified by 'archive_name'
 * to the 'files' list.
 * NOTE: This function is most obviously relevant to implementing minitar's list
 * operation, but think about how you can reuse it for the update operation.
 * This function should return 0 upon success or -1 if an error occurred.
 */
int get_archive_file_list(const char *archive_name, file_list_t *files);

/*
 * Write each file contained within the archive identified by 'archive_name'
 * as a new file to the current working directory.
 * If there are multiple versions of the same file present in the archive,
 * then only the most recently added version should be present as a new file
 * at the end of the extraction process.
 * This function should return 0 upon success or -1 if an error occurred.
 */
int extract_files_from_archive(const char *archive_name);

#endif
