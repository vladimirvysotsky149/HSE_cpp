#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct{
  int dir;
  int reg;
  int lnk;
  int sock;
  int chr;
  int fifo;
  int blk;
}tDirCounters;

void printCounter(const char* name, const int val){
  if(val > 0){
    fprintf(stdout, "Amount of %s files is %d\n", name, val);
  }
}

int main(int argc, char *argv[]){
  tDirCounters cntrs = {0,0,0,0,0,0,0};

  char* path = getcwd(NULL, 0);

  if(path == NULL) 
    perror("Error\n");

  DIR *dir = opendir(path);
  struct dirent *file = readdir(dir);

  while(file!=NULL){

    struct stat sb;
    lstat(file->d_name, &sb);
    
    if(file->d_name[0]!='.'){
      
      switch ((sb.st_mode) & __S_IFMT)
      {
      case __S_IFDIR:
        cntrs.dir++;
        break;
      case __S_IFREG:
        cntrs.reg++;
        break;
      case __S_IFCHR:
        cntrs.chr++;
        break;
      case __S_IFBLK:
        cntrs.blk++;
        break;
      case __S_IFIFO:
        cntrs.fifo++;
        break;
      case __S_IFLNK:
        cntrs.lnk++;
        break;
      case __S_IFSOCK:
        cntrs.sock++;
        break;
      default:
        break;
      }
    }
    file=readdir(dir);
  }

  printCounter("REGULAR", cntrs.reg);
  printCounter("DIRECTORY", cntrs.dir);
  printCounter("SYMBOLIC LINK", cntrs.lnk);
  printCounter("FIFO/PIPE", cntrs.fifo);
  printCounter("SOCKET", cntrs.sock);
  printCounter("BLOCK", cntrs.blk);
  printCounter("CHARACTER", cntrs.chr);


  free(path);

  return 0;
}

