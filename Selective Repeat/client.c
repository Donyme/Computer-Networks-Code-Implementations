#include <stdio.h>		
#include <sys/socket.h>		
#include <arpa/inet.h>		
#include <stdlib.h>		
#include <string.h>		
#include <unistd.h>		
#include <errno.h>
#include <memory.h>
#include <signal.h> 
struct gbnpacket
{
  int type;
  int seq_no;
  int length;
  char data[8];
};	

void Update (int ignored);

int main (int argc, char *argv[])
{
  char buffer[40];		
  buffer[39] = '\0';		
  int sock;			
  struct sockaddr_in gbnServAddr;	
  struct sockaddr_in gbnClntAddr;	
  unsigned int cliAddrLen;	
  unsigned short gbnServPort;	
  int recvMsgSize;		
  int packet_rcvd = -1;		
  struct sigaction myAction;
  double lossRate=0.4;		
  bzero (buffer, 40);		
  gbnServPort = 1234;	
  int chunkSize = 8;	
  srand48(123456789);
  if ((sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    printf("socket() failed\n");
  
  /*INITIALISATION*/
  memset (&gbnServAddr, 0, sizeof (gbnServAddr));	
  gbnServAddr.sin_family = AF_INET;	
  gbnServAddr.sin_addr.s_addr = htonl (INADDR_ANY);	
  gbnServAddr.sin_port = htons (gbnServPort);	
  
  /*ADDRESS BINDING*/
  if (bind (sock, (struct sockaddr *) &gbnServAddr, sizeof (gbnServAddr)) < 0)
    printf("bind() failed\n");
  myAction.sa_handler = Update;
  if (sigfillset (&myAction.sa_mask) < 0)
    printf("sigfillset failed\n");
  myAction.sa_flags = 0;
  if (sigaction (SIGALRM, &myAction, 0) < 0)
    printf ("sigaction failed for SIGALRM\n");

  /*PROCESSING PART*/
  for (;;)			
    {
      //INITIALISE CURRENT PACKET ON CLIENT SIDE
      cliAddrLen = sizeof (gbnClntAddr);
      struct gbnpacket currPacket; 
      memset(&currPacket, 0, sizeof(currPacket));

      //RECEIVE MESSAGE FROM BINDED ADDRESS
      recvMsgSize = recvfrom (sock, &currPacket, sizeof (currPacket), 0, (struct sockaddr *) &gbnClntAddr, &cliAddrLen);
      currPacket.type = ntohl (currPacket.type);
      currPacket.length = ntohl (currPacket.length); 
      currPacket.seq_no = ntohl (currPacket.seq_no);

      //PROCESS THE MESSAGE READ
      if (currPacket.type == 4) 
	{
          int slen=strlen(buffer)-1;
          while(buffer[slen]=='x')
	     slen--;
          buffer[slen+1]='\0';
	  printf ("Message is: %s\n", buffer);
	  struct gbnpacket ackmsg;
	  ackmsg.type = htonl(8);
	  ackmsg.seq_no = htonl(0);
	  ackmsg.length = htonl(0);
	  if (sendto(sock, &ackmsg, sizeof (ackmsg), 0,(struct sockaddr *) &gbnClntAddr,cliAddrLen) != sizeof (ackmsg))
	  {
	      printf("Error sending delete ack\n"); 
	  }
	  alarm (7);
	  while (1)
	    {
	      while ((recvfrom (sock, &currPacket, sizeof (int)*3+chunkSize, 0,(struct sockaddr *) &gbnClntAddr,
				&cliAddrLen))<0)
		{
		  if (errno == EINTR)	
		    {
		      /* never reached */
		      exit(0);
		    }
		  else
		    ;
		}
	      if (ntohl(currPacket.type) == 4) /* respond to more delete messages */
		{
		  ackmsg.type = htonl(8);
		  ackmsg.seq_no = htonl(0);
		  ackmsg.length = htonl(0);
		  if (sendto(sock, &ackmsg, sizeof (ackmsg), 0, (struct sockaddr *) &gbnClntAddr, cliAddrLen) != sizeof (ackmsg))
		    {
		      /* send delete ack */
		      printf("Error sending tear-down ack\n");
		    }
		}
	    }
	  printf ("recvfrom() failed\n");
	}
  
      //IF NOT ALL ASSEMBLED TAKE DECISION TO DROP OR STORE
      else
	{
	  /* drop packets randomly */
	  if(lossRate > drand48())
		continue; 
	  printf ("Received Packet No.=%d and Text=%s\t", currPacket.seq_no, currPacket.data);
	  /* CORRECT THEN INCREMENT ACK*/
	  if (currPacket.seq_no == packet_rcvd + 1)
	    {
	      packet_rcvd++;
	      int buff_offset = chunkSize * currPacket.seq_no;
	      memcpy (&buffer[buff_offset], currPacket.data,currPacket.length);
	    }
        
          //SEND ACK
	  printf ("\tSend Ack for Packet No.=%d\n", packet_rcvd);
	  struct gbnpacket currAck; 
	  currAck.type = htonl (2); 
	  currAck.seq_no = htonl (packet_rcvd);
	  currAck.length = htonl(0);
	  if (sendto (sock, &currAck, sizeof (currAck), 0, (struct sockaddr *) &gbnClntAddr,cliAddrLen) != sizeof (currAck))
	    printf("sendto() sent a different number of bytes than expected\n");
	}
    }
  
}

//IF TIMEOUT,EXIT
void Update (int ignored) 
{
  exit(0);
}
