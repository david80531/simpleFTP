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
#define MAX_CONNECTION 5
#define PORT 8888

void connection_handler(int);
void hello_msg_handler(int);
void file_listing_handler(int, char*);
void file_sending_handler(int, char[]);
void file_upload_handler(int, char[]);
int checkdir(char*);

int main(){
  int ser_fd;
  struct sockaddr_in ser_addr;

  int cli_fd;
  struct sockaddr_in cli_addr;
  socklen_t addr_len;
  pid_t child_pid;

  ser_fd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&ser_addr, sizeof(ser_addr));
  ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  ser_addr.sin_port = htons(PORT);
  ser_addr.sin_family = AF_INET;

  if(bind(ser_fd, (struct sockaddr*) &ser_addr, sizeof(ser_addr))<0){
    perror("Bind Socket Failed\n");
    exit(1);
  }

  if(listen(ser_fd, MAX_CONNECTION)<0){
    perror("Listen Socket Failes\n");
    exit(1);
  }

  printf("Welcome to file transfer server\n");
  printf("Max connection set to %d\n", MAX_CONNECTION);
  printf("Listening on %s:%d\n", inet_ntoa(ser_addr.sin_addr), PORT);
  printf("Waiting for clients...\n");

  addr_len = sizeof(cli_addr);
  while(1){
    cli_fd = accept(ser_fd, (struct sockaddr*)&cli_addr, (socklen_t*) &addr_len);
    printf("[INFO] Connection accepted (id: %d)\n", cli_fd);
    printf("[INFO] Client is from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    child_pid=fork();
    if(child_pid==0){     // in child process
      connection_handler(cli_fd);
      printf("[INFO] Client is disconnected (id: %d)\n", cli_fd);
      printf("[INFO] Disconnected client is %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
      close(cli_fd);
      break;
    } else{               // in parent process
      close(cli_fd);
    }
  }
  close(ser_fd);
  return 0;
}

void connection_handler(int sockfd){
  int i;
  char filename[MAX_SIZE];
  char buf[MAX_SIZE];
  char localPath[MAX_SIZE];
  char path[MAX_SIZE];
  memset(path, '\0', MAX_SIZE);
  memset(filename, '\0', MAX_SIZE);
  memset(buf, '\0', MAX_SIZE);
  memset(localPath, '\0', MAX_SIZE);
  sprintf(localPath, "./server_storage");
  mkdir(localPath, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);

  while(1){
    if(write(sockfd, localPath, strlen(localPath))<0){     //write path to client
      perror("Write Failed!\n");
      exit(1);
    }
    memset(buf, '\0', MAX_SIZE);
    if(read(sockfd, buf, MAX_SIZE)==0){                    //receive command from client

      printf("Client(%d) is terminated\n", sockfd);

    } else if(strcmp(buf, "cd")==0){                               //change directory
      memset(buf, '\0', MAX_SIZE);
      if((read(sockfd, buf, MAX_SIZE))<0){
        perror("Read Failed!\n");
        exit(1);
      }
      memset(path, '\0', MAX_SIZE);
      strcpy(path, localPath);
      strcat(path, "/");
      strcat(path, buf);

      if((strcmp(buf, ".."))==0){                       //check for ".."
        if((strcmp(localPath, "./server_storage"))==0){
          strcpy(localPath, "./server_storage");
        } else{
          for(i = strlen(localPath) - 1; i>=0; i--){
            if(localPath[i]=='/')
              break;
            else
              localPath[i] = '\0';
          }
          localPath[i] = '\0';
          //printf("%s\n", localPath);
          //printf("i: %d\n", i);

        }
      } else{

          if(checkdir(path)){                       //check if file or dir exist
            strcpy(localPath, path);
          } else {
            printf("No Such Directory\n");
          }

      }

    }else if(strcmp(buf, "ls")==0){                         //listing files
      file_listing_handler(sockfd, localPath);


    }else if(strcmp(buf, "mkdir")==0){
      memset(buf, '\0', MAX_SIZE);
      if(read(sockfd, buf, MAX_SIZE)<0){                    //receive filename from client
        perror("Read Failed!\n");
        exit(1);
      }
      strcpy(path, localPath);
      strcat(path, "/");
      strcat(path, buf);
      if((mkdir(path, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH))==0){
        printf("Make Directory Success!\n");
      } else{
        printf("Make Directory Failed.\n");
      }


    }else if(strcmp(buf, "up")==0){                         //upload
      memset(buf, '\0', MAX_SIZE);
      if(read(sockfd, buf, MAX_SIZE)<0){
        perror("Read Failed!\n");
        exit(1);
      }

      if(strcmp(buf, "1")==0){
        memset(buf, '\0', MAX_SIZE);
        read(sockfd, buf, MAX_SIZE);       //read the file name

        memset(path, '\0', MAX_SIZE);
        strcat(path, localPath);
        strcat(path, "/");
        strcat(path, buf);
        file_upload_handler(sockfd, path);
      } else{
        printf("No such files to upload!\n");
      }

    }else if(strcmp(buf, "dl")==0){                         //download
      memset(buf, '\0', MAX_SIZE);
      if(read(sockfd, buf, MAX_SIZE)<0){                    //receive filename from client
        perror("Read Failed!\n");
        exit(1);
      }
      memset(path, '\0', MAX_SIZE);
      strcpy(path, localPath);
      strcat(path, "/");
      strcat(path, buf);
      //printf("%s\n", path);
      //printf("%s\n", localPath);
      if(checkdir(path)){                       //check if file or dir exist
        memset(buf, '\0', MAX_SIZE);
        strcpy(buf, "1");
        write(sockfd, buf, strlen(buf));
        sleep(1);
        file_sending_handler(sockfd, path);
      } else {
        memset(buf, '\0', MAX_SIZE);
        //printf("%s\n", path);
        //printf("%s\n", localPath);
        strcpy(buf, "0");
        write(sockfd, buf, strlen(buf));
        sleep(1);
        printf("No such file to download!\n");;
      }



    }else if(strcmp(buf, "es")==0){
      break;

    }else if(strcmp(buf, "else")==0){
      memset(buf, '\0', MAX_SIZE);
      sprintf(buf, "No such command!\n");
      write(sockfd, buf, strlen(buf));
      memset(buf, '\0', MAX_SIZE);

    } else {

    }
    sleep(1);
  }
  return;
}



void file_listing_handler(int sockfd, char *dir) {
  DIR* pDir;
  struct dirent* pDirent = NULL;
  char buf[MAX_SIZE];
  char path[MAX_SIZE];
  memset(path, '\0', MAX_SIZE);
  memset(buf, '\0', MAX_SIZE);
  printf("[INFO] List file to client\n");

  strcpy(path, dir);
  if((pDir=opendir(path))==NULL){
    perror("open directory failed\n");
    exit(1);
  }
  while((pDirent = readdir(pDir))!=NULL){
    //if(strcmp(pDirent->d_name, ".")==0||strcmp(pDirent->d_name, "..")==0)
      //continue;

    strcat(pDirent->d_name, "\n");
    strcat(buf, pDirent->d_name);
  }
  if(write(sockfd, buf, strlen(buf))<0){
    perror("Writing Failed\n");
    exit(1);
  }
  closedir(pDir);
}

int  checkdir(char* dir){
  struct stat chk;
  char path[MAX_SIZE];
  memset(path, '\0', MAX_SIZE);
  strcpy(path, dir);


  if((stat(path, &chk))==-1){
  //  printf("0000000\n");
    return 0;
  } else {
  //  printf("1111111\n");
    return 1;
  }
}


void file_sending_handler(int sockfd, char path[]){
  char buf[MAX_SIZE];
  memset(buf, '\0', MAX_SIZE);

  int file_size = 0;
  int write_sum = 0;
  int write_bytes = 0;

  FILE *fp;

  fp = fopen(path, "rb");

  if(fp){

    memset(buf, '\0', MAX_SIZE);
    sprintf(buf, "Downloading from %s.....\n", path);    // send start message to client
    if(write(sockfd, buf, sizeof(buf)) < 0){
      printf("Writing bytes failed\n");
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    memset(buf, '\0', MAX_SIZE);
    sprintf(buf, "%d", file_size);
    write(sockfd, buf, sizeof(buf));                        //send filesize to client

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
    sprintf(buf, "%s", "Download successfully!-------------\n");
    write(sockfd, buf, strlen(buf));


  }else{
    sprintf(buf, "[ERROR] Can not download the file!\n");
    write(sockfd, buf, sizeof(buf));
    return;
  }

}

void file_upload_handler(int sockfd, char path[]){

  FILE *fp;

  int file_size = 0;
  int read_sum = 0;
  int read_bytes = 0;

  char buf[MAX_SIZE];


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
