#include <stdio.h>
#include <string.h>

#include "file_list.h"
#include "minitar.h"

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
        return 1;
        //The given code has this set to 0, but the project page says errors should return 1 so I changed it
    }
    char err_msg[512];
    file_list_t files;
    file_list_init(&files);
    for(int x=4; x<argc;x++){
        if(file_list_add(&files,argv[x])!=0){
            printf(err_msg, 512, "Failed to add file to list %s", argv[x]);
            file_list_clear(&files);
            return 1;
        }
    }
    //This creates the file list struct and populates it with arguments from the console (If any files are mentioned)

    if (strcmp("-c", argv[1]) == 0) {
        //Simple create job see minitar.c for more info
            if(create_archive(argv[3],&files)!=0){
                //free and return if error
                //error messages are found in minitar.c commands
                perror("-c Create option failed");
                file_list_clear(&files);
                return 1;
            }
        //This command is capable of creating an empty archive, only consisting of 1024 0 bytes. 
        //However, the minitar.h file says we can ignore this case and assume all files exist.
        }
    if (strcmp("-a", argv[1]) == 0) {
        //Simple append job see minitar.c for more info
            if(append_files_to_archive(argv[3],&files)!=0){
                //free and return if error
                //error messages are found in minitar.c commands
                perror("-a Append option failed");
                file_list_clear(&files);
                return 1;
            }
        }
    if (strcmp("-t", argv[1]) == 0) {
            file_list_clear(&files);
            //make sure there are no existing files to mess things up, as we can pass in irrelevant arguments
            if(get_archive_file_list(argv[3],&files)!=0){
                //free and return if error
                //error messages are found in minitar.c commands
                perror("-t Option failed");
                file_list_clear(&files);
                return 1;
            }
            else{
                //if successful we iterate through the file list and print them all out. 
                node_t *current_node = files.head;
                while(current_node!=NULL){
                    printf("%s\n", current_node->name);
                    current_node = current_node->next;
                }
            }
        }
    if (strcmp("-u", argv[1]) == 0) {
        //make a separate list of files inside of archive
        file_list_t currentlyinarchive;
        file_list_init(&currentlyinarchive);
        //Get file list will error if archive does not exist, as will append
            if(get_archive_file_list(argv[3],&currentlyinarchive)!=0){
                //populate the list, if successful continue, otherwise free and terminate.
                file_list_clear(&files);
                file_list_clear(&currentlyinarchive);
                //error messages are found in minitar.c commands
                return 1;
            }
            else{
                if(file_list_is_subset(&files, &currentlyinarchive)!=1){
                        printf("Error: One or more of the specified files is not already present in archive");
                        file_list_clear(&currentlyinarchive);
                        file_list_clear(&files);
                        //If there is a file in the arguments that DNE in the archive free and terminate.
                        return 1;
                    }
                //if all are present then we update
                if(append_files_to_archive(argv[3],&files)!=0){
                    perror("Failed to append files exiting...");
                    file_list_clear(&currentlyinarchive);
                        file_list_clear(&files);
                        return 1;
                }
                }
            file_list_clear(&currentlyinarchive);
            //clear the new file list (in archive not arguments.)
            }
    if (strcmp("-x", argv[1]) == 0) {
        //if x run extract files, if error return 1 and clear. else nothing. 
        if(extract_files_from_archive(argv[3])!=0){
            //Only the most recent updated file is extracted
            file_list_clear(&files);
            return 1;
        }
    }
    //if no errors, clear files and return 0
    file_list_clear(&files);
    return 0;
}
