#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  /* contains a number of basic derived types */
#include <sys/socket.h> /* provide functions and data structures of socket */
#include <arpa/inet.h>  /* convert internet address and dotted-decimal notation */
#include <netinet/in.h> /* provide constants and structures needed for internet domain addresses*/
#include <unistd.h>     /* `read()` and `write()` functions */
#include <dirent.h>     /* format of directory entries */
#include <sys/stat.h>   /* stat information about files attributes */

#define MAX_SIZE 2048
#define PORT 8888

void connection_handler(int);
void list_files(int);
void passName(int, char*);
void file_download_handler(int, char*);
void file_sending_handler(int, char*);
int checkdir(char*);


int main(int argc, char **argv){
  int cli_fd;
  struct sockaddr_in ser_addr;

  cli_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(cli_fd<0){
    perror("Client Create Socket Failed!\n");
    exit(1);
  }

  bzero(&ser_addr, sizeof(ser_addr));
  ser_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  ser_addr.sin_family = AF_INET;
  ser_addr.sin_port = htons(PORT);

  if (connect(cli_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0) {
    perror("Connect failed");
    exit(1);
  }

  connection_handler(cli_fd);
  close(cli_fd);
  return 0;

}

void connection_handler(int sockfd){
  char instruct[MAX_SIZE];
  char buf[MAX_SIZE];
  char path[MAX_SIZE];
  char * cmd;
  char op[MAX_SIZE];
  memset(buf, '\0', MAX_SIZE);
  memset(instruct, '\0', MAX_SIZE);
  /* create download dir */
  sprintf(path, "./download");
  mkdir(path, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);

  printf("[cd]:\n");
  printf("type cd + directory name to change directory\n");
  printf("[ls]:\n");
  printf("type ls to list all files in directory\n");
  printf("[mkdir]:\n");
  printf("typr mkdir + directory name to make new directory in server\n");
  printf("[up]:\n");
  printf("type up + filename to upload file to server\n");
  printf("[dl]:\n");
  printf("type dl + filename to download file from server\n");
  printf("[es]:\n");
  printf("type es to exit\n\n\n");
  printf("[Waring] You must download the file before uploading!\n");
  printf("[Waring] You must download the file before uploading!\n");
  printf("[Waring] You must download the file before uploading!\n\n\n");



  //printf("Type your command:\n");
  while(1){
    memset(buf, '\0', MAX_SIZE);
    read(sockfd, buf, MAX_SIZE);               //read path from server
    printf("file path <%s>$", buf);


    memset(instruct, '\0', MAX_SIZE);
    fgets(instruct, MAX_SIZE, stdin);

    if(instruct[0]==' '|| instruct[0]=='\n'|| instruct[0]=='\t'){  // 防呆處理
      memset(buf, '\0', MAX_SIZE);
      sprintf(buf, "else");
      if( write(sockfd, buf, strlen(buf)) < 0){
        perror("Write Bytes Failed\n");
        exit(1);
      }
      memset(buf, '\0', MAX_SIZE);

      if( (read(sockfd, buf, MAX_SIZE)) > 0){
        printf("%s", buf);
      }
      continue;
    }


    cmd = strtok(instruct, " \n");
    strcpy(op, cmd);


    if(strcmp(op, "cd")==0){
      memset(buf, '\0', MAX_SIZE);
      sprintf(buf, "cd");
      if( write(sockfd, buf, strlen(buf)) < 0){
        perror("Write Bytes Failed\n");
        exit(1);
      }
      cmd = strtok(NULL, "\n");
      passName(sockfd, cmd);


    } else if(strcmp(op, "ls")==0){
      list_files(sockfd);


    } else if(strcmp(op,"mkdir")==0){
      memset(buf, '\0', MAX_SIZE);
      sprintf(buf, "mkdir");
      if( write(sockfd, buf, strlen(buf)) < 0){
        perror("Write Bytes Failed\n");
        exit(1);
      }
      cmd = strtok(NULL, "\n");
      passName(sockfd, cmd);



    } else if(strcmp(op, "up")==0){             //upload operation
      memset(buf, '\0', MAX_SIZE);
      sprintf(buf, "up");
      if( write(sockfd, buf, strlen(buf)) < 0){
        perror("Write Bytes Failed\n");
        exit(1);
      }

      cmd = strtok(NULL, "\n");
      if(checkdir(cmd)){
        memset(buf, '\0', MAX_SIZE);
        sprintf(buf, "1");
        write(sockfd, buf, strlen(buf));
        sleep(1);
        passName(sockfd, cmd);
        file_sending_handler(sockfd, cmd);
      }else{
        memset(buf, '\0', MAX_SIZE);
        sprintf(buf, "0");
        write(sockfd, buf, strlen(buf));
        sleep(1);
        printf("No such files in client!\n");
      }



    } else if(strcmp(op, "dl")==0){             //download operation
      memset(buf, '\0', MAX_SIZE);
      sprintf(buf, "dl");
      if( write(sockfd, buf, strlen(buf)) < 0){
        perror("Write Bytes Failed\n");
        exit(1);
      }

      cmd = strtok(NULL, "\n");
      passName(sockfd, cmd);

      memset(buf, '\0', MAX_SIZE);
      if( read(sockfd, buf, MAX_SIZE) < 0){
        perror("Read Bytes Failed\n");
        exit(1);
      }
      //sleep(1);
      //printf("%s\n", buf);

      if(strcmp(buf, "1")==0){                   //file exist, start download
        file_download_handler(sockfd, cmd);
      }else{
        printf("No such files\n");
      }

    } else if(strcmp(op, "es")==0){
      memset(buf, '\0', MAX_SIZE);
      sprintf(buf, "es");
      if( write(sockfd, buf, strlen(buf)) < 0){
        perror("Write Bytes Failed\n");
        exit(1);
      }

      break;

    } else{
      memset(buf, '\0', MAX_SIZE);
      sprintf(buf, "else");
      if( write(sockfd, buf, strlen(buf)) < 0){
        perror("Write Bytes Failed\n");
        exit(1);
      }
      memset(buf, '\0', MAX_SIZE);

      if( (read(sockfd, buf, MAX_SIZE)) > 0){
        printf("%s", buf);
      }
    }
  }
  return;
}


void list_files(int sockfd){
  char buf[MAX_SIZE];
  memset(buf, '\0', MAX_SIZE);
  sprintf(buf, "ls");
  if( write(sockfd, buf, strlen(buf)) < 0){
    perror("Write Bytes Failed\n");
    exit(1);
  }

  memset(buf, '\0', MAX_SIZE);
  if((read(sockfd, buf, MAX_SIZE))>0){
    printf("Server Files:\n");
    printf("%s", buf);
    printf("------------------------\n");

  }

}

void passName(int sockfd, char *d_name){
  char buf[MAX_SIZE];
  memset(buf, '\0', MAX_SIZE);
  strcpy(buf, d_name);
  if((write(sockfd, buf, strlen(buf)))<0){
    perror("Write Bytes Failed 2\n");
    exit(1);
  }
  sleep(1);
}

void file_download_handler(int sockfd, char *f_name){

  FILE *fp;

  int file_size = 0;
  int read_sum = 0;
  int read_bytes = 0;

  char path[MAX_SIZE];
  char buf[MAX_SIZE];
  char filename[MAX_SIZE];
  memset(path, '\0', MAX_SIZE);
  memset(filename, '\0', MAX_SIZE);
  strcpy(filename, f_name);

  sleep(1);
  /* receive start message */

  memset(buf, '\0', MAX_SIZE);
  read(sockfd, buf, MAX_SIZE);
  printf("%s\n", buf);

  /* receive file_size */

  memset(buf, '\0', MAX_SIZE);
  read(sockfd, buf, MAX_SIZE);
  file_size = atoi(buf);
  printf("file size: %d bytes\n", file_size);

  //specify path
  sprintf(path, "./download/%s", filename);

  fp = fopen(path, "wb");
  if(fp){
    read_sum = 0;
    while(read_sum < file_size){
      memset(buf, '\0', MAX_SIZE);
      read_bytes = read(sockfd, buf, MAX_SIZE);
      if(read_bytes > 0){
        fwrite(&buf, sizeof(char), read_bytes, fp);
        read_sum += read_bytes;
      }
    }

    fclose(fp);

    //receive complete message

    memset(buf, '\0', MAX_SIZE);
    read(sockfd, buf, MAX_SIZE);
    printf("%s\n", buf);



  }else{
    perror("Allocate memory failed");
    return;
  }

}

int  checkdir(char* dir){
  struct stat chk;
  char path[MAX_SIZE];
  memset(path, '\0', MAX_SIZE);
  sprintf(path, "./download/%s", dir);


  if((stat(path, &chk))==-1){
    //printf("0000000\n");
    return 0;
  } else {
    //printf("1111111\n");
    return 1;
  }
}

void file_sending_handler(int sockfd, char *f_name){

  char path[MAX_SIZE];
  char buf[MAX_SIZE];
  memset(buf, '\0', MAX_SIZE);
  memset(buf, '\0', MAX_SIZE);

  int file_size = 0;
  int write_sum = 0;
  int write_bytes = 0;

  sprintf(path, "./download/%s", f_name);

  FILE *fp;

  fp = fopen(path, "rb");

  if(fp){

    memset(buf, '\0', MAX_SIZE);
    sprintf(buf, "Uploading from %s.....\n", path);    // send start message to server
    if(write(sockfd, buf, sizeof(buf)) < 0){
      printf("Writing bytes failed\n");
    }

    printf("Uploading from %s.....\n\n\n", path);



    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    memset(buf, '\0', MAX_SIZE);
    sprintf(buf, "%d", file_size);
    write(sockfd, buf, sizeof(buf));                        //send filesize to server

    printf("file size:%d bytes\n\n\n", file_size);

    write_bytes = 0;
                                                            //start sending file
    while(write_sum < file_size){

      memset(buf, '\0', MAX_SIZE);
      write_bytes = fread(&buf, sizeof(char), MAX_SIZE, fp);

      if(write(sockfd, buf, write_bytes) > 0){
        write_sum += write_bytes;
      }

    }

    fclose(fp);

    sleep(3);

    memset(buf, '\0', MAX_SIZE);
    sprintf(buf, "%s", "Upload successfully!-------------\n");
    write(sockfd, buf, strlen(buf));

    printf("%s\n", buf);

  }else{
    sprintf(buf, "[ERROR] Can not upload the file!\n");
    write(sockfd, buf, sizeof(buf));
    return;
  }



}
