/* Send Multicast Datagram code example. */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <liquid/liquid.h>

#define datalen 1024
#define file_name_len 100
#define end_len 12
#define package_num_len 15

struct in_addr localInterface;
struct sockaddr_in groupSock;
int sd;


int main (int argc, char *argv[ ])
{
  char end_signal[end_len] = "endOFtheFILE";
  char file[file_name_len];
  memset(file,'\0',file_name_len);
  strcat(file,argv[1]);

  /* Create a datagram socket on which to send. */
  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sd < 0)
  {
    perror("Opening datagram socket error");
    exit(1);
  }
  else
    printf("Opening the datagram socket...OK.\n");
 
  /* Initialize the group sockaddr structure with a */
  /* group address of 225.1.1.1 and port 5555. */
  memset((char *) &groupSock, 0, sizeof(groupSock));
  groupSock.sin_family = AF_INET;
  groupSock.sin_addr.s_addr = inet_addr("226.1.1.1");
  groupSock.sin_port = htons(4321);
 
  /* Disable loopback so you do not receive your own datagrams.
  {
  char loopch = 0;
  if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0)
  {
  perror("Setting IP_MULTICAST_LOOP error");
  close(sd);
  exit(1);
  }
  else
  printf("Disabling the loopback...OK.\n");
  }
  */
 
  /* Set local interface for outbound multicast datagrams. */
  /* The IP address specified must be associated with a local, */
  /* multicast capable interface. */
  localInterface.s_addr = inet_addr("127.0.0.1");
  if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0)
  {
    perror("Setting local interface error");
    exit(1);
  }
  else
    printf("Setting the local interface...OK\n");
  /* Send a message to the multicast group specified by the*/
  /* groupSock sockaddr structure. */
  /*int datalen = 1024;*/

  FILE *f;
  f = fopen(file,"rb");
  if(f == NULL)
    perror("file open fail!\n");

  /*send the file name*/
  sendto(sd, file, file_name_len, 0, (struct sockaddr*)&groupSock, sizeof(groupSock));
  int numbyte;
  int package_num = 0;
  unsigned char package[package_num_len];
  unsigned char databuf[datalen];
  unsigned char msg[datalen + package_num_len + 1];
  while(!feof(f)){
    
    ++package_num;
    memset(package,'\0',package_num_len);
    sprintf(package,"%d",package_num);
    memset(databuf,'\0',datalen);
    
    numbyte = fread(databuf, sizeof(unsigned char), sizeof(databuf), f);
    memset(msg,'\0',datalen + package_num_len + 1);
    strncat(msg, package, sizeof(package));
    while(strlen(msg) != package_num_len)
      strncat(msg, "-",sizeof(unsigned char));
    strncat(msg, databuf,sizeof(databuf));
    /*encode*/
    // simulation parameters
    unsigned int n = sizeof(msg); // original data length (bytes)
    // compute size of encoded message
    unsigned int k = n;             // (no encoding yet)

    // create arrays
    unsigned char msg_org[n];       // original data message
    unsigned char msg_enc[k];       // encoded/received data message
    unsigned int i;
    // "encode" message (copy to msg_enc)
    for (i=0; i<n; i++) msg_enc[i] = msg[i];
    // corrupt encoded message (flip bit)
    msg_enc[0] ^= 0x01;

    if(sendto(sd, msg, numbyte + package_num_len, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0)
    {
      perror("Sending datagram message error");
    }
    else{
      printf("`````%d``````\n",numbyte + package_num_len);
      printf("Sendinf msg: %s OK\n",msg);
    }
  }
  /* Try the re-read from the socket if the loopback is not disable
  if(read(sd, databuf, datalen) < 0)
  {
    perror("Reading datagram message error\n");
    close(sd);
    exit(1);
  }
  else
  {
    printf("Reading datagram message from client...OK\n");
    printf("The message is: %s\n", databuf);
  }
  */
  fclose(f);
  /*send file ending signal*/
  memset(databuf,'\0',datalen);
  strcat(databuf,end_signal);
  sendto(sd, databuf, end_len, 0, (struct sockaddr*)&groupSock, sizeof(groupSock));
  /*send number of package */
  memset(databuf,'\0',datalen);
  strcat(databuf,package);
  while(strlen(databuf) != package_num_len)
    strcat(databuf,"\n");
  sendto(sd, databuf, package_num_len, 0, (struct sockaddr*)&groupSock, sizeof(groupSock));
  close(sd);
  return 0;
}

//gcc multicast_server.c -o sever
