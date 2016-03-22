/*!								www.mycal.net
-----------------------------------------------------------------------------
udpsend.c

Sends a UDP message either unicast or broadcast.

(c)2009-2011 Mycal.net
-----------------------------------------------------------------------------
*/
#include "config.h"


//setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv);
int
set_sock_recv_timeout(SOCKET lsock, int secs)
{
    int ret=-1;
    struct timeval tv;

    tv.tv_sec = secs;
    tv.tv_usec = 0;
    if ( (ret=setsockopt(lsock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) ) < 0)
    {

    }
//    DEBUG1("set recv timeout ret %d\n",ret);
    return(ret);
}


void usage(int argc, char **argv)
{
  printf("usage: %s [-p port] [-h host] [-v(erbose)] [-b(roadcast)] [-?(this message)] message to send\n",argv[0]);
  printf("	Defaults port=1024, host=127.0.0.1, verbose off, broadcast off\n");
  exit(0);
}

int main(int argc, char *argv[])
{
	int					ci,verbose=0,send_broadcast=0;
	int					ret,sd,slen;
	U16					destport;
	struct sockaddr_in			server;
	struct hostent				*host_info;
	IPADDR					ip;
	char					message[4096];
	int broadcast = 1;	
    int receive = 0;

	// Set defaults
	ip.ipb1=127;
	ip.ipb2=0;
	ip.ipb3=0;
	ip.ipb4=1;
	destport=1024;

	//------------------------------------------------------------------
	// Argument Scan command line args, first for -h -b
	//------------------------------------------------------------------

	for(ci=1;ci<argc;ci++)
	{
		if(0 == strncmp(argv[ci], "-p", 2))
		{
			// Get port
			ci++;
			if(argc==ci)
				usage(argc,argv);

			destport=atoi(argv[ci]);
		}
		else if(0 == strncmp(argv[ci], "-h", 2))
		{
			U8		*subst;		
	
			// Get host, check if IP or Name
			ci++;
			if(argc==ci)
				usage(argc,argv);
			
			if(isalpha(argv[ci][0]))
			{
				// convert name to IP
				host_info=gethostbyname(argv[ci]);
				if(host_info!=0)
				{
					ip.ip32=*((unsigned long*)host_info->h_addr);
				}
				else
				{
					printf("Failed to resolve %s\n",argv[ci]);
					exit(0);
				}
				//if(verbose)
                                //        printf("Sending to %s or %d.%d.%d.%d\n",argv[ci],ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4);
			}	
			else
			{
				// get proxy target IP			
				subst=(U8 *) strtok(argv[ci],".\n");
				if(strlen((char *) subst))
					ip.ipb1=atoi((char *) subst);
				subst=(U8 *) strtok(NULL,".\n");
				if(strlen((char *) subst))
					ip.ipb2=atoi((char *) subst);
				subst=(U8 *) strtok(NULL,".\n");
				if(subst)
					ip.ipb3=atoi((char *) subst);
				subst=(U8 *) strtok(NULL,".\n");
				if(subst)
					ip.ipb4=atoi((char *) subst);
				//if(verbose)
				//	printf("Sending to %d.%d.%d.%d\n",ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4);
			}
		}
        else if(0 == strncmp(argv[ci], "-b", 2))
        {
            send_broadcast=1;
        	ip.ipb1=255;
        	ip.ipb2=255;
        	ip.ipb3=255;
        	ip.ipb4=255;
	    }
		else if(0 == strncmp(argv[ci], "-v", 2))
		{
			// Verbose
			verbose=1;
			printf("verbose on\n");
		}
		else if(0 == strncmp(argv[ci], "-r", 2))
		{
		    receive=1;
		}
		else if(0 == strncmp(argv[ci], "-?", 2))
		{
			// print help
			usage(argc,argv);
		}
		else
		{
			// No more args break
			break;
		}
	}


	if(argc > ci)
	{
		int i;

		// Standard Config
		message[0]=0;
		memset(message,0,4096);
		//
		// Build Message
		//
		for(i=ci;i<argc;i++)
		{

			// Check if we fit in tweet
			// stlen argv+1 for space
			if((strlen(argv[i])+strlen(message)+1)<4096)
			{
				if(!strlen(message))
					strcpy(message,argv[i]);
				else
				{
					strcat(message," ");
					strcat(message,argv[i]);
				}
			}
			else
			{
				// Build end/Partial
				int	tlen=(4096-1)-strlen(message);

				if(tlen)
				{
					if(!strlen(message))
					{
						strncpy(message,argv[i],tlen);
					}
					else
					{
						if(tlen>3)
						{
							strcat(message," ");
							strncat(message,argv[i],tlen-1);
						}
					}
				}
				break;
			}
		}
	}


	if(strlen(message))
	{
		strcat(message,"\n");
		sd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sd == -1)
		{
			printf("Could not create socket.\n");
			exit(1);
		}

		// Broadcast?
		if(send_broadcast)
		{
             if(verbose)
				 printf("Setting socket to broadcast.\n");
			// this call is what allows broadcast packets to be sent:
			if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof broadcast) == -1)
			{
				perror("Could not set broadcast (Not Root?)\n");
				exit(1);
			} 
		}


        //memset((void *)&server, '\0', sizeof(struct sockaddr_in));
       // server.sin_family       = AF_INET;
        //server.sin_addr.s_addr  = INADDR_ANY;
        //server.sin_port         = 0;
        //ret=bind(sd, (struct sockaddr *)&client, sizeof(struct sockaddr_in));

        memset((void *)&server, '\0', sizeof(struct sockaddr_in));
		server.sin_family		= AF_INET;
		server.sin_addr.s_addr		= ip.ip32;
		server.sin_port			= htons((U16)(destport));

		ret=sendto(sd, (char *)message, strlen(message), 0, (struct sockaddr *)&server, sizeof(struct sockaddr));

		if(ret<0)
		{
			if(verbose)
				printf("failed to send message %s\n",message);
			exit(1);
		}
		else
		{
			if(verbose)
			{
				if(send_broadcast)
					printf("sent brodcast to %d.%d.%d.%d:%d the message : %s\n",ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4,destport,message);
				else
					printf("sent to %d.%d.%d.%d:%d the message : %s\n",ip.ipb1,ip.ipb2,ip.ipb3,ip.ipb4,destport,message);
			}
           
            if(receive)
            {
                set_sock_recv_timeout(sd, 2);
                memset((void *)&server, '\0', sizeof(struct sockaddr_in));
			    slen=sizeof(struct sockaddr_in);
			    ret=recvfrom(sd, (char *)message, 1024-1, 0, (struct sockaddr *)&server, (socklen_t *) &slen);
                if(ret>0)
                    printf("%s\n",message);
                else
                    printf("ERR: timeout\n");
            }


			exit(0);	
		}
	}
	else
	{
		usage(argc,argv);
	}

    

	exit(0);

}

