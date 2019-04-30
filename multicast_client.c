/* Receiver/client multicast Datagram example. */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define datalen 1023
#define end_len 12
#define file_name_len 100
#define package_num_len 15

struct sockaddr_in localSock;
struct ip_mreq group;
int sd;


int main(int argc, char *argv[])
{
  char end_signal[end_len] = "endOFtheFILE";
  /* Create a datagram socket on which to receive. */
  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sd < 0)
  {
    perror("Opening datagram socket error");
    exit(1);
  }
  else
    printf("Opening datagram socket....OK.\n");
 
  /* Enable SO_REUSEADDR to allow multiple instances of this */
  /* application to receive copies of the multicast datagrams. */
  {
    int reuse = 1;
    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
    {
      perror("Setting SO_REUSEADDR error");
      close(sd);
      exit(1);
    }
    else
      printf("Setting SO_REUSEADDR...OK.\n");
  }
 
  /* Bind to the proper port number with the IP address */
  /* specified as INADDR_ANY. */
  memset((char *) &localSock, 0, sizeof(localSock));
  localSock.sin_family = AF_INET;
  localSock.sin_port = htons(4321);
  localSock.sin_addr.s_addr = INADDR_ANY;
  if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock)))
  {
    perror("Binding datagram socket error");
    close(sd);
    exit(1);
  }
  else
    printf("Binding datagram socket...OK.\n");
 
  /* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
  /* interface. Note that this IP_ADD_MEMBERSHIP option must be */
  /* called for each local interface over which the multicast */
  /* datagrams are to be received. */
  group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
  group.imr_interface.s_addr = inet_addr("127.0.0.1");
  if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
  {
    perror("Adding multicast group error");
    close(sd);
    exit(1);
  }
  else
    printf("Adding multicast group...OK.\n");

  /*create the recv dir*/
  struct stat st = {0};
  if(stat("recv", &st) == -1)
    mkdir("recv", 0755);

  /*get the file name from server*/
  char file[file_name_len];
  memset(file,'\0',file_name_len);
  char file_name[file_name_len];
  memset(file_name,'\0',file_name_len);
  if(read(sd, file_name, sizeof(file_name)) < 0)
    printf("read file name fail!\n");
  strcat(file,"recv/");
  strcat(file,file_name);
  FILE *f;
  f = fopen(file,"wb");
  if(f == NULL)
    perror("fopen fail");
  
  /* Read from the socket. */
  int numbyte;
  char package[package_num_len+1];
  char databuf[datalen];
  char msg[datalen+package_num_len];
  while(1){
    memset(msg,'\0',datalen+package_num_len);
    numbyte = read(sd, msg, sizeof(msg));
    printf("````````%d``````````\n",numbyte);
    if(strncmp(msg,end_signal,end_len) == 0)
      break;
    else if(numbyte < 0)
    {
      perror("Reading datagram message error");
      break;
    }
    else
    {
      memset(databuf,'\0',datalen);
      memset(package,'\0',package_num_len);
      printf("The message from multicast server is: %s\n", msg);
      memcpy(package, msg, package_num_len*sizeof(char));
      memcpy(databuf, msg + package_num_len, (numbyte-package_num_len)*sizeof(char));
      fwrite(&databuf, sizeof(char), numbyte-package_num_len, f);
      //printf("the char cut : %s ",package);
    }
  }
  /* total package number */
  read(sd, msg, sizeof(msg));

  close(sd);

  return 0;
}

//gcc multicast_client.c -o client
