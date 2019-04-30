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
#include <liquid/liquid.h>

#define datalen 1023
#define end_len 12
#define file_name_len 100
#define package_num_len 15
#define encode_len 2000
#define ip_len 16

struct sockaddr_in localSock;
struct ip_mreq group;
int sd;


int main(int argc, char *argv[])
{
  char end_signal[end_len] = "endOFtheFILE";
  char ip[ip_len];
  memset(ip,'\0',ip_len);
  strncat(ip,argv[1],strlen(argv[1]));

  /* Create a datagram socket on which to receive. */
  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sd < 0)
  {
    perror("Opening datagram socket error");
    exit(1);
  }
 
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
 
  /* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
  /* interface. Note that this IP_ADD_MEMBERSHIP option must be */
  /* called for each local interface over which the multicast */
  /* datagrams are to be received. */
  group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
  group.imr_interface.s_addr = inet_addr(ip);
  if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
  {
    perror("Adding multicast group error");
    close(sd);
    exit(1);
  }

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
  int package_count=0 ;
  unsigned char package[package_num_len+1];
  unsigned char databuf[datalen];
  unsigned char encode_msg[encode_len];
  unsigned char msg[datalen+package_num_len + package_num_len];
  unsigned char len[package_num_len+1];
  char real_len[package_num_len];
  fec_scheme fs = LIQUID_FEC_HAMMING74;   // error-correcting scheme
  while(1){
    memset(msg,'\0',datalen+package_num_len + package_num_len);
    numbyte = read(sd, encode_msg, sizeof(encode_msg));
    if(strncmp(encode_msg,end_signal,end_len) == 0)
      break;
    else if(numbyte < 0)
    {
      perror("Reading datagram message error");
      break;
    }
    else
    {
      ++package_count;
      unsigned int n = datalen + package_num_len + package_num_len + 1;
      fec q = fec_create(fs,NULL);
      fec_decode(q, n, encode_msg, msg);
      memset(databuf,'\0',datalen);
      memset(package,'\0',package_num_len);
      memset(len,'\0',package_num_len);

      numbyte = sizeof(msg);
      memcpy(package, msg, package_num_len*sizeof(unsigned char));
      memcpy(len, msg + package_num_len, package_num_len*sizeof(unsigned char));
      printf("len = %s\n",len);
      int k=0;
      int count=0;
      memset(real_len,'\0',package_num_len);
      for(k=0;k<package_num_len;++k){
        if(len[k]=='-')
          break;
      }
      memcpy(real_len, len, k*sizeof(unsigned char));
      int real_size = atoi(real_len);
      memcpy(databuf, msg + package_num_len + package_num_len, (numbyte-package_num_len-+ package_num_len)*sizeof(unsigned char));
      fwrite(databuf, sizeof(unsigned char), real_size, f);
      fec_destroy(q);
    }
  }
  /* total package number */
  read(sd, msg, sizeof(msg));
  int total = atoi(msg);
  close(sd);
  printf("package_count=%d  total = %d\n",package_count,total);
  double package_loss_rate = (double)(total-package_count) / total;
  printf("package loss rate = %lf\n", package_loss_rate);

  return 0;
}

//gcc multicast_client.c -o client -lliquid
//./client 127.0.0.1
