#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>

int main(int argc , char *argv[])
{

    //socket creating
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        perror("create a socket\n");
    }

    //socket connecting

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;

    //localhost test
    info.sin_addr.s_addr = inet_addr("127.0.0.1");
    info.sin_port = htons(8700);


    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        perror("Connection error");
    }

    char buf[buf_size];
    FILE *fp;
    char file_path[100] = {};
    char path[name_size];
    memset(path,'\0',name_size);
    //get the file_path send by serverS
    read(sockfd, file_path, sizeof(file_path));
    strcat(path,"recv/");
    strcat(path,file_path);
    //if there is not a recv folder, create one
    struct stat st={0};
    if(stat("recv",&st) == -1)
        mkdir("recv",0755);
    if ((fp = fopen(path, "wb")) == NULL){
        perror("fopen");
        close(sockfd);
        return -1;
    }
    char get_file_size[size_len];
    memset(get_file_size,'\0',size_len);
    read(sockfd, get_file_size, sizeof(get_file_size));
    get_file_size[strlen(get_file_size)]='\0';
    int file_size = 0;
    file_size = atoi(get_file_size);
    //Receive file from server
    int numbytes;
    int sum = 0;
    while(1){
        numbytes = read(sockfd, buf, sizeof(buf));
        //calculate the percentage of receieve file
        sum += numbytes;
        if(numbytes <= 0){
            break;
        }
        numbytes = fwrite(&buf, sizeof(char), numbytes, fp);
    }
    fclose(fp);
    //Send a message to server
    /*
    char message[] = {"Hi there"};
    char receiveMessage[100] = {};
    send(sockfd,message,sizeof(message),0);
    recv(sockfd,receiveMessage,sizeof(receiveMessage),0);

    printf("%s",receiveMessage);
    */
    printf("close Socket\n");
    close(sockfd);
    return 0;
}
