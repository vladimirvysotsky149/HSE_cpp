#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

typedef enum{
  mode_none,
  mode_add,
  mode_read,
  mode_delete
}tMode;

#define MN_END1 0xFF
#define MN_END2 0xD9

#define START_OF_MSG 0x16

void printCharArr(const char* arr, int size){
  for(int i = 0; i < size; i++){
    printf("%x ", arr[i]);
  }
  printf("\n");
}

int getEndOfJpegOffset(const int fd){
  
  int total_size = lseek(fd, 0, SEEK_END);
  //printf("Size is %d\n", total_size);
  lseek(fd, 3, SEEK_SET);
  total_size-=2;
  char x;
  char prev;
  
  while (--total_size > 0){
    read(fd, &x, 1);
    //printf("%x\n", x);
    if((x == (char)0xD9)&&(prev==(char)0xFF)){
      printf("Found jpeg end code\n");
      break;
    }
    prev = x;  
  }
  int result = lseek(fd, 0, SEEK_CUR);
  //lseek(fd, -16, SEEK_END);
  //char arr[16];
  //read(fd, arr, 16);
  //printf("Size is %d\n", total_size);
  //printCharArr(arr, 16);
  return result;
}

int main(int argc, char* argv[]){
    tMode mode = mode_none;

    /* Check args */
    if(argc != 4){
        fprintf(stderr, "Invalid arguments\n");
    }

    if(strcmp(argv[1],"add") == 0){
      mode = mode_add;
    }else if(strcmp(argv[1],"read") == 0){
      mode = mode_read;
    }else if(strcmp(argv[1],"delete") == 0){
      mode = mode_delete;
    }else{
      fprintf(stderr, "Unknown mode %s\n", argv[1]);
    }

    char secret_arr[4] = {1,2,3,4};
    int filedes = 0;

    filedes = open(argv[3], O_RDWR);
    if(filedes >= 0){
      int hasMsg = 0;
      char m = 0;
      char len = 0;
      int end_of_jpeg_section = getEndOfJpegOffset(filedes);
      int end_of_file = lseek(filedes, 0, SEEK_END);

      //printf("%d\n", end_of_jpeg_section);
      //fprintf(stdout, "Offset: %d\n", lseek(filedes, 0, SEEK_END));
      if((end_of_jpeg_section)==end_of_file){
        printf("Clear file\n");
      }
      else{
        
        lseek(filedes, end_of_jpeg_section, SEEK_SET);
        read(filedes, &m, 1);
        //printf("m is %x\n", m);
        if(m == (char)START_OF_MSG)
        {
          printf("Has message\n");
          hasMsg = 1;
        }
        else{
          printf("Has deleted message\n");
          hasMsg = 2;
        }
      }

      if(mode == mode_add){
        //printf("Add mode\n");
        if(hasMsg == 1){
          fprintf(stderr, "Invalid mode\n");
        }
        else{
          m = START_OF_MSG;
          //fprintf(stdout, "Offset: %d\n", lseek(filedes, end_of_jpeg_section, SEEK_SET));
          lseek(filedes, end_of_jpeg_section, SEEK_SET);
          write(filedes, &m, 1);
          len = strlen(argv[2]);
          write(filedes, &len, 1);
          write(filedes, argv[2], strlen(argv[2]));
        }
      }
      else if(mode == mode_read){
        //printf("Read mode\n");
        if(hasMsg == 1){
          //fprintf(stdout, "Offset: %d\n", lseek(filedes, end_of_jpeg_section+1, SEEK_SET));
          lseek(filedes, end_of_jpeg_section+1, SEEK_SET);
          read(filedes, &len, 1);
          //printf("len is %x\n", len);
          char* msg = malloc(len);
          read(filedes, msg, len);
          fprintf(stdout, "Data: %s\n", msg);
          free(msg);
        }
        else{
          fprintf(stderr, "Message is empty\n");
        }
      }
      else if(mode == mode_delete){
        printf("Delete mode\n");
        if(hasMsg == 1){
          for(int i = 0; i < 4; i++){
        //    secret_arr[i] = '\0';
          }
          //fprintf(stdout, "Offset: %d\n", lseek(filedes, end_of_jpeg_section, SEEK_SET));
          lseek(filedes, end_of_jpeg_section, SEEK_SET);
          m = 0;
          write(filedes, &m, 1);
          read(filedes, &len, 1);
          //printf("len is %x\n", len);
          //write(filedes, secret_arr, 4);
          //fprintf(stdout, "Data: %x %x %x %x\n", secret_arr[0], secret_arr[1], secret_arr[2], secret_arr[3]);
        }
        else{
          fprintf(stderr, "Message is empty\n");
        }
      }
    }

    return 0;
}