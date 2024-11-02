#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

typedef enum{
  none,
  code,
  decode
}tMode;

#define SECRET_CODE   0x16
#define SECRET_LENGH  0x04

int main(int argc, char *argv[]){
  tMode mode = none;
  char* path = "";

  if(argc != 3){
    fprintf(stderr, "Invalid arguments! argc = %d\n", argc);
    for(int i = 0; i < argc; i++){
      printf("%s\n", argv[i]);
    }
    return -1;
  }
  if(strcmp(argv[1],"code") == 0){
    mode = code;
  }else if(strcmp(argv[1],"decode") == 0){
    mode = decode;
  }else{
    fprintf(stderr, "Unknown mode %s\n", argv[1]);
  }

  path = argv[2];

  int filedes = 0;
  char e_ident[SECRET_LENGH];
  char secret_arr[SECRET_LENGH];

  filedes = open(path, O_RDWR);
  if(filedes >= 0){
    fchmod(filedes, S_IRUSR|S_IRGRP|S_IROTH|S_IRWXU);
    lseek(filedes, 0, SEEK_SET);
    read(filedes, e_ident, SECRET_LENGH);

  for(int i = 0; i < SECRET_LENGH; i++){
    secret_arr[i] = SECRET_CODE;
    //printf("%x", e_ident[i]);
  }
  //printf("\n");
    if(mode == code){
      if((e_ident[0] == SECRET_CODE)&&(e_ident[1] == SECRET_CODE)){
        fprintf(stderr, "Can't code this file. This file was coded earlier!\n");
        return 0;
      }
      lseek(filedes, 0, SEEK_END);
      write(filedes, e_ident, SECRET_LENGH);  

      lseek(filedes, 0, SEEK_SET);
      write(filedes, secret_arr, SECRET_LENGH);

          fprintf(stdout, "Code operation was successful\n");
    }
    else if(mode == decode){
      if((e_ident[0] != SECRET_CODE)||(e_ident[1] != SECRET_CODE)){
        fprintf(stderr, "Can't decode this file. This file need to be coded!\n");
        return 0;
      }
      lseek(filedes, -4, SEEK_END);
      read(filedes, e_ident, SECRET_LENGH);  

      //printf("%x %x %x %x\n", e_ident[0], e_ident[1], e_ident[2],e_ident[3]);

      lseek(filedes, 0, SEEK_SET);
      write(filedes, e_ident, SECRET_LENGH);
          
      fprintf(stdout, "Decode operation was successful\n");
    }
  }
  else{
    fprintf(stderr, "Can't open file %s\n", path);
  }
  close(filedes);

  return 0;
}

