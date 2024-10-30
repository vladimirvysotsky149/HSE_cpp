#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]){
  int type_cntrs[7] = {0,-2,0,0,0,0,0};

  char* path = getcwd(NULL, 0);

  if(path == NULL) 
    perror("Error\n");

  DIR *dir = opendir(path);
  struct dirent *file = readdir(dir);
  //int filedes = 0;
  //char e_ident[16];

  while(file!=NULL){
  //printf("%s\n", file->d_name);
  //if(file->d_name[0] != '.'){
    struct stat sb;
    stat(file->d_name, &sb);
    //printf("%x\n", sb.st_mode);
    if(S_ISDIR(sb.st_mode))
      type_cntrs[1]++;
    else if(S_ISLNK(sb.st_mode))
      type_cntrs[2]++;
    else if(S_ISCHR(sb.st_mode))
      type_cntrs[3]++;
    else if(S_ISBLK(sb.st_mode))
      type_cntrs[4]++;
    else if(S_ISFIFO(sb.st_mode))
      type_cntrs[5]++;
    else if(S_ISSOCK(sb.st_mode))
      type_cntrs[6]++;
    else if(S_ISREG(sb.st_mode))
      type_cntrs[0]++;

    //filedes = open(file->d_name, O_RDONLY);
    //if(filedes >= 0){
    //  read(filedes, e_ident, 16);
    //}  
    //close(filedes);
  //}
    file=readdir(dir);
  }

  printf("REG: %d\n", type_cntrs[0]);
  printf("DIR: %d\n", type_cntrs[1]);
  printf("LNK: %d\n", type_cntrs[2]);
  printf("FIFO: %d\n", type_cntrs[5]);
  printf("SOCK: %d\n", type_cntrs[6]);
  printf("BLK: %d\n", type_cntrs[4]);
  printf("CHR: %d\n", type_cntrs[3]);


  free(path);

  return 0;
}

