#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]){
  int mode = 0;
  char* path = "";

  if(argc != 3){
    printf("Invalid arguments! argc = %d\n", argc);
    for(int i = 0; i < argc; i++){
      printf("%s\n", argv[i]);
    }
    return -1;
  }
  sscanf(argv[1], "%d", &mode);
  if((mode != 1)&&(mode != 2)){
    printf("Invalid mode! mode is %d\n", mode);
    return -1;
  }
  path = argv[2];

  int filedes = 0;
  char e_ident[4];

  filedes = open(path, O_RDWR);
  if(filedes >= 0){
    fchmod(filedes, S_IRUSR|S_IRGRP|S_IROTH|S_IRWXU);
    lseek(filedes, 0, SEEK_SET);
    read(filedes, e_ident, 4);
   for(int i = 0; i < 4; i++){
     //printf("%x ", e_ident[i]);
     if(mode == 1)
       e_ident[i]+=i;
     else
       e_ident[i]-=i;
    }
    //printf("\n");
    lseek(filedes, 0, SEEK_SET);
    write(filedes, e_ident, 4);
  }
  else{
    printf("Can't open file %s\n", path);
  }
  close(filedes);

  return 0;
}

