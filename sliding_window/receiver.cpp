#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<unistd.h>
int main()
{
        int sfd,lfd,len,choice;
        char str[20],str1[20],err[20];
        struct sockaddr_in saddr,caddr;
        sfd=socket(AF_INET,SOCK_STREAM,0);
        if(sfd<0)
                perror("FdError");
        bzero(&saddr,sizeof(saddr));
        saddr.sin_family=AF_INET;
        saddr.sin_addr.s_addr=INADDR_ANY;
        saddr.sin_port=htons(5465);
        connect(sfd,(struct sockaddr*)&saddr,sizeof(saddr));
        for(;;)
        {
                read(sfd,str,20);
                if(!strcmp(str,"exit"))
                {
                        printf("Exiting\n");
                        break;
                }
                printf("\nReceived--%s\nDo you want to report an error(1(Yes)/0(No))",str);
                scanf("%d",&choice);
                if(!choice)
                        write(sfd,"-1",sizeof("-1"));
                else
                {
                        printf("Enter the index no. of the frame where error is:\n");
                        scanf("%s",err);
                        write(sfd,err,sizeof(err));
                        read(sfd,str,20);
                        printf("\nReceived the re-transmitted frames--%s\n",str);
                }
        }
}
