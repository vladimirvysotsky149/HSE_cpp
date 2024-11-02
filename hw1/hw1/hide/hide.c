#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#define HIDDE_PATH_NAME     "/hide"

int main(int argc, char *argv[]){
    int retVal = 0;
    char* dir_path = getcwd(NULL, 0);

    /* Check args */
    if(argc != 2){
        fprintf(stderr, "Invalid arguents");
    }

    /* Create folder name */
    int hide_path_len = strlen(dir_path) + strlen(HIDDE_PATH_NAME);
    char* hide_path = malloc(hide_path_len);    
    if(dir_path == NULL) 
        perror("Error\n");

    hide_path = strncat(hide_path, dir_path, strlen(dir_path));
    hide_path = strncat(hide_path, HIDDE_PATH_NAME, strlen(HIDDE_PATH_NAME));
  
    /* Change access */
    DIR *dirk = opendir(hide_path);
    if(ENOENT == errno){
      fprintf(stderr, "Miss folder\n");
      free(hide_path);
      return -1;
    }
    else if(dirk == NULL){
      fprintf(stderr, "Unavailable folder\n");
      chmod(hide_path, S_IRUSR|S_IRGRP|S_IROTH|S_IRWXU);
    }
    else{
        closedir(dirk);
    }

    /* Check directory and find file */
    DIR *dir = opendir(hide_path);  
    if(dir == NULL){
     fprintf(stderr, "%s directory is unavailable or miss\n", HIDDE_PATH_NAME);
     free(hide_path);
     return -1;
    }   
    fprintf(stdout, "Dir is %s\n", dir_path);   
    struct dirent *file = readdir(dir);
    int fCnt = 0;   
    while(file != NULL){
      if(strcmp(file->d_name, argv[1])==0){
          fCnt++;
          break;
      }
      file = readdir(dir);    
    }

    if(fCnt==0){
      fprintf(stderr, "File %s is not found\n", argv[1]);
      retVal = -1;
    }
    else
    {
        /* Hide file */
        int len = strlen(file->d_name);
        char* new_name = malloc(hide_path_len + len + 2);
        char* old_name = malloc(hide_path_len + len + 1);
        
        old_name = strncat(old_name, hide_path, hide_path_len);
        old_name = strncat(old_name, "/", 1);
        old_name = strncat(old_name, file->d_name, len);
        fprintf(stdout, "Old file name: %s\n", old_name);
        
        new_name = strncat(new_name, hide_path, hide_path_len);
        new_name = strncat(new_name, "/.", 2);
        new_name = strncat(new_name, file->d_name, len);
        fprintf(stdout, "New file name: %s\n", new_name);
        
        if(rename(old_name, new_name)!=0)
            fprintf(stderr, "Rename error\n", new_name);
        free(new_name);
        free(old_name);
    }
      
    chmod(hide_path, S_IXGRP|S_IXOTH|S_IRWXU);
    closedir(dir);
    free(hide_path);
    return retVal;
}
