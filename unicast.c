#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<time.h>

#define buf_size 64
#define name_size 100
#define msg_size 64
#define size_len 10

int tcp_client(char* ip, int port){
    //socket building
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    //AF_INET: transfer by internet(IPv4)
    //SOCK_STREAM: TCP
    //SOCK_dgram: UDP
    if (sockfd == -1){
        perror("create a socket\n");
        return -1;
    }
    //socket connection
    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(ip);  //inet_addr() convert IP type from string to integer
    info.sin_port = htons(port);
    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        perror("Connection error");
        close(sockfd);
        return -1;
    }
    //Open file
    char type[name_size];
    memset(type,'\0',name_size);
    read(sockfd, type, sizeof(type));
    time_t raw_time;
    struct tm* timeinfo;
    time(&raw_time);

    if(!strncmp(type,"PIC",3)){
        //if it is a file
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
        int log_data = 0;
        int last_data_log = 0;
        file_size = atoi(get_file_size);
        //Receive file from server
        int numbytes;
        int sum = 0;
        while(1){
            numbytes = read(sockfd, buf, sizeof(buf));
            //calculate the percentage of receieve file
            sum += numbytes;
            log_data = ((double)sum/file_size)*100;
            //get the time
            timeinfo = localtime(&raw_time);
            if((log_data/5)*5 != last_data_log){
                for(int log_num = last_data_log+5; log_num <= (log_data/5)*5; log_num += 5)
                    //printf("%d%c %d/%d/%d %d:%d:%d\n",log_num,'%',timeinfo->tm_year+1900,timeinfo->tm_mon,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
            }
            if(numbytes <= 0){
                break;
            }
            numbytes = fwrite(&buf, sizeof(char), numbytes, fp);
            last_data_log = (log_data/5)*5;
        }
        fclose(fp);
    }
    else{
        //if it is a msg
        char msg[buf_size];
        memset(msg,'\0',buf_size);
        read(sockfd, msg, sizeof(msg));
        printf("Msg from server:\n%s\n",msg);
    }
    close(sockfd);
    return 0;
}

int tcp_server(char* ip,int port, char file_path[msg_size]){
    //socket building
    char message[msg_size];
    int sockfd = 0, new_fd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1){
        printf("Fail to create a socket.");
    }
    //socket connection
    struct sockaddr_in serverInfo,clientInfo;
    socklen_t addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));  //initialize,all bits=0
    serverInfo.sin_family = PF_INET;    //Ipv4
    serverInfo.sin_addr.s_addr = inet_addr(ip);//INADDR_ANY;   //any address, kernel decide
    serverInfo.sin_port = htons(port);  //htons() convert endian
    int on=1;
    if((setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on))) < 0)
    {
        perror("setsockopt fail\n");
        return -1;
    }
    if(bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo)) < 0)
    {
        perror("bind fail\n");
        return -1;
    }
    listen(sockfd,5);

    char type[name_size];
    memset(type,'\0',name_size);
    //Get file stat
    struct stat filestat;
    if(lstat(file_path, &filestat) < 0){ 
        new_fd = accept(sockfd,(struct sockaddr*) &clientInfo, &addrlen);
        //if it is a msg
        strcat(type,"MSG");
        memset(message,'\0',msg_size);
        strcat(message,file_path);
        write(new_fd, type, sizeof(type));
        write(new_fd,message,sizeof(message));
        close(new_fd);
        close(sockfd);
        return 0;
    }
    //if it is a picture
    FILE *f;
    char buf[buf_size], file_size[size_len];
    int numbytes;
    memset(file_size,'\0',size_len);
	f = fopen(file_path, "rb");
    if ((new_fd = accept(sockfd, (struct sockaddr*)&clientInfo, &addrlen)) == -1 ){
        perror("accept\n");
        close(new_fd);
        close(sockfd);
        return 0;
	}
    strcat(type,"PIC");
    write(new_fd, type, sizeof(type));
    char path[name_size];
    memset(path,'\0',name_size);
    strcat(path,file_path);
    write(new_fd, path, sizeof(path));
    sprintf(file_size,"%lu",filestat.st_size);
    file_size[strlen(file_size)] = '\0';
    write(new_fd, file_size, sizeof(file_size));
    //Sending file
	while(!feof(f)){
        //read the file and send to client
		numbytes = fread(buf, sizeof(char), sizeof(buf), f);   //read the file to buffer
		numbytes = write(new_fd, buf, numbytes);
	}
    printf("Done\n");
    fclose(f);
    close(new_fd);
    close(sockfd);
    return 0;
}

int udp_client(char* ip, int port){
    //udp don't need to connect
    //socket building
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_DGRAM , 0);
    if (sockfd == -1){
        perror("create a socket\n");
        return -1;
    }
    //socket connection
    struct sockaddr_in info, server_info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(ip);  //inet_addr() convert IP type from string to integer
    info.sin_port = htons(port);
    socklen_t addrlen = sizeof(info);
    if(sendto(sockfd, "hi there", strlen("hi there"), 0, (struct sockaddr*)&info, addrlen) < 0) 
        perror("Could not send datagram!!\n");
    //Open file
    char type[name_size];
    memset(type,'\0',name_size);
    recvfrom(sockfd,type,sizeof(type),0,(struct sockaddr *)&info,&addrlen);
    time_t raw_time;
    struct tm* timeinfo;
    time(&raw_time);
    if(!strncmp(type,"PIC",3)){
        //if receive a file
        char buf[buf_size];
        FILE *fp;
        char file_path[100] = {};
        char path[name_size];
        memset(path,'\0',name_size);
        if(sendto(sockfd, "hi there", strlen("hi there"), 0, (struct sockaddr*)&info, addrlen) < 0) 
            perror("Could not send datagram!!\n");
        recvfrom(sockfd,file_path,sizeof(file_path),0,(struct sockaddr *)&info,&addrlen);
        //add the recv directory on the path name
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
        //receive the file size and change into integer
        char get_file_size[size_len];
        memset(get_file_size,'\0',size_len);
        if(sendto(sockfd, "hi there", strlen("hi there"), 0, (struct sockaddr*)&info, addrlen) < 0) 
            perror("Could not send datagram!!\n");
        recvfrom(sockfd,get_file_size,sizeof(get_file_size),0,(struct sockaddr *)&info,&addrlen);
        get_file_size[strlen(get_file_size)]='\0';
        int file_size = 0;
        int log_data = 0;
        int last_data_log = 0;
        file_size = atoi(get_file_size);
        //Receive file from server
        int numbytes;
        int sum = 0;
        char endMsg[msg_size];
        memset(endMsg,'\0',msg_size);
        strcat(endMsg,"ending");
        while(1){
            //recieve the file from server and write into another file
            if(sendto(sockfd, "hi there", strlen("hi there"), 0, (struct sockaddr*)&info, addrlen) < 0) 
                perror("Could not send datagram!!\n");
            numbytes = recvfrom(sockfd,buf,sizeof(buf),0,(struct sockaddr *)&info,&addrlen);
            sum += numbytes;
            if((!strncmp(buf,endMsg,strlen(endMsg))) || numbytes <= 0){
                break;
            }
            log_data = ((float)sum/file_size)*100;
            timeinfo = localtime(&raw_time);
            if((log_data/5)*5 != last_data_log){
                for(int log_num = last_data_log+5; log_num <= (log_data/5)*5; log_num += 5)
                    //printf("%d%c %d/%d/%d %d:%d:%d\n",log_num,'%',timeinfo->tm_year+1900,timeinfo->tm_mon,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
            }
            numbytes = fwrite(&buf, sizeof(char), numbytes, fp);
            last_data_log = (log_data/5)*5;
        }
        fclose(fp);
    }
    else{
        //if it is a msg
        char msg[buf_size];
        memset(msg,'\0',buf_size);
        if(sendto(sockfd, "hi there", strlen("hi there"), 0, (struct sockaddr*)&info, addrlen) < 0) 
            perror("Could not send datagram!!\n");
        recvfrom(sockfd,msg,sizeof(msg),0,(struct sockaddr *)&info,&addrlen);
        printf("Message from server:\n%s\n",msg);
    }
    close(sockfd);
    return 0;
}

int udp_server(char* ip,int port, char file_path[msg_size]){
    //udp don't need to listen and accept
    //socket building
    char inputBuffer[256] = {};
    char message[msg_size];
    int sockfd = 0,forClientSockfd = 0, new_fd = 0;
    sockfd = socket(AF_INET , SOCK_DGRAM , 0);
    //AF_INET: transfer by internet(IPv4)
    //SOCK_STREAM: TCP
    //SOCK_dgram: UDP
    if (sockfd < 0){
        perror("Fail to create a socket.\n");
        return -1;
    }
    //socket connection
    struct sockaddr_in serverInfo,clientInfo;
    socklen_t addrlen = sizeof(clientInfo);
    bzero(&serverInfo,sizeof(serverInfo));  //initialize,all bits=0
    serverInfo.sin_family = PF_INET;    //Ipv4
    serverInfo.sin_addr.s_addr = inet_addr(ip);//INADDR_ANY;   //any address, kernel decide
    serverInfo.sin_port = htons(port);  //htons() convert endian
    int ret = bind(sockfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));
    if(ret < 0){
        perror("bind fail\n");
        return -1;
    }
    FILE *f;
    struct stat filestat;
    char buf[buf_size], feedback[buf_size], type[name_size];
    int numbytes;
    //Get file stat
    memset(type,'\0',name_size);
    memset(feedback,'\0',buf_size);
    if(lstat(file_path, &filestat) < 0){ 
        //if it is a msg
        strcat(type,"MSG");
        memset(message,'\0',msg_size);
        strcat(message,file_path);
        addrlen = sizeof(clientInfo);
        if(recvfrom(sockfd, feedback,buf_size,0, (struct sockaddr*)&clientInfo, &addrlen) < 0)
            perror("recv fail\n");
        if(sendto(sockfd, type, name_size, 0, (struct sockaddr*)&clientInfo, addrlen) < 0) 
            perror("Could not send datagram!!\n");
        
        if(recvfrom(sockfd, feedback,buf_size,0, (struct sockaddr*)&clientInfo, &addrlen) < 0)
            perror("recv fail\n");
        if(sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr*)&clientInfo, addrlen) < 0) 
            perror("Could not send datagram!!\n");
        close(new_fd);
        close(sockfd);
        return 0;
    }
    //if it is a picture
	f = fopen(file_path, "rb");
    strcat(type,"PIC");
    if(recvfrom(sockfd, feedback,buf_size,0, (struct sockaddr*)&clientInfo, &addrlen) < 0)
            perror("recv fail\n");
    if (sendto(sockfd, type, name_size, 0, (struct sockaddr*)&clientInfo, addrlen) < 0) 
        perror("Could not send datagram!!\n");
    //send file path to client
    char path[name_size];
    memset(path,'\0',name_size);
    strcat(path,file_path);
    if(recvfrom(sockfd, feedback,buf_size,0, (struct sockaddr*)&clientInfo, &addrlen) < 0)
        perror("recv fail\n");
    if (sendto(sockfd, path, name_size, 0, (struct sockaddr*)&clientInfo, addrlen) < 0) 
        perror("Could not send datagram!!\n");
    //send file size to client
    char file_size[size_len];
    memset(file_size,'\0',size_len);
    sprintf(file_size,"%lu",filestat.st_size);
    file_size[strlen(file_size)] = '\0';
    if(recvfrom(sockfd, feedback,buf_size,0, (struct sockaddr*)&clientInfo, &addrlen) < 0)
        perror("recv fail\n");
    if (sendto(sockfd, file_size, size_len, 0, (struct sockaddr*)&clientInfo, addrlen) < 0) 
        perror("Could not send datagram!!\n");
    //Sending file
	while(!feof(f)){
		numbytes = fread(buf, sizeof(char), sizeof(buf), f);   //read the file to buffer
        if(recvfrom(sockfd, feedback,buf_size,0, (struct sockaddr*)&clientInfo, &addrlen) < 0)
            perror("recv fail\n");
		if (sendto(sockfd, buf, numbytes, 0, (struct sockaddr*)&clientInfo, addrlen) < 0) 
            perror("Could not send datagram!!\n");
	}
    char endMsg[msg_size];
    memset(endMsg,'\0',msg_size);
    strcat(endMsg,"ending");
    if(recvfrom(sockfd, feedback,buf_size,0, (struct sockaddr*)&clientInfo, &addrlen) < 0)
        perror("recv fail\n");
	if (sendto(sockfd, endMsg, msg_size, 0, (struct sockaddr*)&clientInfo, addrlen) < 0) 
        perror("Could not send datagram!!\n");
    printf("Done\n");
    fclose(f);
    close(new_fd);
    close(sockfd);
    return 0;
}

int main(int argc, char* argv[]){

    char* protocol = argv[1];
    char* mode = argv[2];
    char* ip = argv[3];
    int port = atoi(argv[4]);
    char* tcp = "tcp";
    char* udp = "udp";
    char* send = "send";
    char* recv = "recv";
    if(argc<5){
        printf("error input\n");
        return 0;
    }
    if(!strncmp(mode,send,strlen(send))){
        char file_path[msg_size];
        memset(file_path,'\0',msg_size);
        for(int i=5;i<argc;++i){
            strcat(file_path,argv[i]);
            if(i != argc-1)
                strcat(file_path," ");
        }
        if(!strncmp(protocol,tcp,strlen(tcp)))
            tcp_server(ip, port, file_path);
        else if(!strncmp(protocol,udp,strlen(udp)))
            udp_server(ip, port, file_path);
        else
            printf("Wrong protocol input\n");
        }
    else if(!strncmp(mode,recv,strlen(recv))){
        if(!strncmp(protocol,tcp,strlen(tcp)))
            tcp_client(ip, port);
        else if(!strncmp(protocol,udp,strlen(udp)))
            udp_client(ip, port);
        else
            printf("Wrong protocol input\n");
    }
    else
        printf("Wrong mode input\n");
    return 0;
}
/*
./lab1_file_transfer tcp send 127.0.0.1 8001 largest.JPG
./lab1_file_transfer tcp recv 127.0.0.1 8001
./lab1_file_transfer udp send 127.0.0.1 8001 Hi there
./lab1_file_transfer udp recv 127.0.0.1 8001
*/
