#include <stdio.h>		
#include <sys/socket.h>		
#include <arpa/inet.h>		
#include <stdlib.h>		
#include <string.h>		
#include <unistd.h>		
#include <errno.h>		
#include <signal.h>		
struct gbnpacket
{
  int type;
  int seq_no;
  int length;
  char data[8];
};

#define TIMEOUT_SECS    3	
#define MAXTRIES        10	

int tries = 0;			
int base = 0;
int windowSize = 0;
int sendflag = 1;


void Update (int ignored);	
int max (int a, int b);		
int min(int a, int b);		

int main (int argc, char *argv[])
{
  int sock,b_size,ibuff;			
  struct sockaddr_in gbnServAddr;	
  struct sockaddr_in fromAddr;	
  unsigned short gbnServPort;	
  unsigned int fromSize;	
  struct sigaction myAction;	
  char *servIP;			
  int respLen;			
  int packet_received = -1;	
  int packet_sent = -1;	
  char buffer[40],ch;
  int sind=0;    
  printf("Enter the message:");
  fflush(stdin);
  while((ch=getchar())!='\n')
   buffer[sind++]=ch;
  buffer[sind]='\0';
  b_size=40-strlen(buffer);
  for(ibuff=0;ibuff<b_size;ibuff++)
  {
	strcat(buffer,"x");
  }
  strcat(buffer,"\0");
  printf("MESSAGE TO BE SENT IS :%s\n",buffer);
  const int datasize = 40;	
  int chunkSize;		
  int nPackets = 0;		
  
  /*INITIALISATION*/
  servIP = "127.0.0.1";		
  chunkSize = 8;	
  gbnServPort = 1234;	
  windowSize = 4;
  if(chunkSize > 8)
  {
    fprintf(stderr, "chunk size must be less than 8\n");
    exit(1);
  }

  nPackets = datasize / chunkSize; 
  if (datasize % chunkSize)
    nPackets++;			
  if ((sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    printf ("socket() failed\n");
  printf ("created socket\n");

  myAction.sa_handler = Update;
  if (sigfillset (&myAction.sa_mask) < 0)	
    printf ("sigfillset() failed\n");
  myAction.sa_flags = 0;

  if (sigaction (SIGALRM, &myAction, 0) < 0)
    printf ("sigaction() failed for SIGALRM\n");

  /*ADDRESS BINDING*/
  memset (&gbnServAddr, 0, sizeof (gbnServAddr));	
  gbnServAddr.sin_family = AF_INET;
  gbnServAddr.sin_addr.s_addr = inet_addr (servIP);	
  gbnServAddr.sin_port = htons (gbnServPort);	

  /*PROCESS AND SEND MESSAGE CHUNKS*/
  while ((packet_received < nPackets-1) && (tries < MAXTRIES))
    {
     
      if (sendflag > 0)
	{
	sendflag = 0;
	  int ctr; 
	  for (ctr = 0; ctr < windowSize; ctr++)
	    {
	      packet_sent = min(max (base + ctr, packet_sent),nPackets-1); 
	      struct gbnpacket currpacket; 
	      if ((base + ctr) < nPackets)
		{
		  memset(&currpacket,0,sizeof(currpacket));
 		  printf ("Sending Packet No.=%d\n",base+ctr/*, packet_sent*/);

		  currpacket.type = htonl (1); 
		  currpacket.seq_no = htonl (base + ctr);
		  int currlength;
		  if ((datasize - ((base + ctr) * chunkSize)) >= chunkSize) 
		    currlength = chunkSize;
		  else
		    currlength = datasize % chunkSize;
		  currpacket.length = htonl (currlength);
		  memcpy (currpacket.data,buffer + ((base + ctr) * chunkSize), currlength);
		  if (sendto(sock, &currpacket, (sizeof (int) * 3) + currlength, 0, (struct sockaddr *) &gbnServAddr,
		       sizeof (gbnServAddr)) !=((sizeof (int) * 3) + currlength))
		    printf("sendto() sent a different number of bytes than expected\n");
		}
	    }
	}
      fromSize = sizeof (fromAddr);
      alarm (TIMEOUT_SECS);	
      struct gbnpacket currAck;
      while ((respLen = (recvfrom (sock, &currAck, sizeof (int) * 3, 0,(struct sockaddr *) &fromAddr,&fromSize))) < 0)
	if (errno == EINTR)	
	  {
	    if (tries < MAXTRIES)	
	      {
		printf ("Timed out, Retrying...\n"/*, MAXTRIES - tries*/);
		break;
	      }
	    else
	      printf ("No Response\n");
	  }
	else
	  printf ("recvfrom() failed\n");

      
      if (respLen)
	{
	  int acktype = ntohl (currAck.type); 
	  int ackno = ntohl (currAck.seq_no); 
	  if (ackno > packet_received && acktype == 2)
	    {
	      printf ("Received Ack\n"); 
	      packet_received++;
	      base = packet_received; 
	      if (packet_received == packet_sent) 
		{
		  alarm (0); 
		  tries = 0;
		  sendflag = 1;
		}
	      else 
		{
		  tries = 0; 
		  sendflag = 0;
		  alarm(TIMEOUT_SECS); 

		}
	    }
	}
    }
  int ctr;
  for (ctr = 0; ctr < 10; ctr++) 
    {
      struct gbnpacket teardown;
      teardown.type = htonl (4);
      teardown.seq_no = htonl (0);
      teardown.length = htonl (0);
      sendto (sock, &teardown, (sizeof (int) * 3), 0,(struct sockaddr *) &gbnServAddr, sizeof (gbnServAddr));
    }
  close (sock); 
  exit (0);
}

void Update (int ignored)	
{
  tries += 1;
  sendflag = 1;
}

int max (int a, int b)
{
  if (b > a)
    return b;
  return a;
}

int min(int a, int b)
{
  if(b>a)
	return a;
  return b;
}
