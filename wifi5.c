/* 
 * File:   main.c
 */

//Header files needed for networking functions
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
//Header file for manipulating strings
#include <string.h>
#include <stdio.h>
//Header file for errno
#include <errno.h>


//////////////////////Variables////////////////////////
int sockfd, newsockfd;
char data;
struct sockaddr_in serv_addr;
struct sockaddr_storage serverStorage; 
socklen_t addr_size;
char flag = 0x01;

////////////////////enum for Wifi tick function/////////////
enum Connection {inSession, restablish} connectStatus;

/////////////////function prototypes//////////////////////
void startServer();
int checkSession();
char getData( int sockfd );


/////////////////////Wifi tick function/////////////////////
void Wifiinterface()
{
/////////////Wifi Transistions written here//////////////
	switch(connectStatus)
	{
	case inSession:
	{
		if( flag == 0x01 )
		{
			connectStatus = inSession;

		}
		else
		{
			connectStatus = restablish;
		}
		break;
	}
	case restablish:
	{
		if( flag = 0x01 )
		{
			connectStatus = inSession;
		}
		else
		{
			connectStatus = restablish;
		}
		break;
	}
	}


////////////////Wifi interface state actions here//////////////////////
	switch (connectStatus)
	{
	case inSession:
	{
		printf("inSession:\n");
		//Waiting for number from app 
		data = getData( newsockfd );

		printf( "got %d\n", data ); //May not be necessary only for testing purposes 
		break;
	}
	case restablish:
	{
		startServer();
		break;
	}
	}
           
}


///////////////Function definitions//////////////////
void startServer()
{
	newsockfd = accept(sockfd, (struct sockaddr*)&serv_addr, &addr_size); 
}

//Same as getData. This function is useless.
int checkSession()
{
	printf("in check session\n");
	char temp = 0xff;

	/*if( write(newsockfd, &temp, sizeof(temp)) )
	{
		printf("Closed");
		close(newsockfd);
		return 1;
	}
	else
	{
		printf("leaving check session");
		return 0;
	}
	*/

	
	int error = write(newsockfd, &temp, sizeof(temp));
	printf("Stuck here");
	if( error == -1)
	{
		printf("Closed");
		close(newsockfd);
		return 1;
	}
	else
	{
		printf("leaving check session");
		return 0;
	}
	
}

char getData( int sockfd ) 
{
	char temp[1];
	ssize_t n;

	if ( (n = read(sockfd,temp,1) ) <= 0 ) // read? Isn't it recv?
	{
		printf("error\n");//error( const_cast<char *>( "ERROR reading from socket") );
		flag = 0x00;
		return 0xff;
	}
	else
	{
		printf("success\n");
		flag = 0x01;
		return temp[0];
	}
	//temp[n] = '\0'; //is this necessary?
	//return temp[0];
}

/*This function is called when a system call fails.
 * It displays a message about the error on stderr and then aborts the program.
 */
//void error(char *msg)
//{
//	perror(msg);
//	exit(0);
//}



int main() 
{
 	int portno = 3024;
	//char data, buffer[1]; //only using 8 bits
	//struct sockaddr_in cli_addr;
	

	printf( "using port #%d\n", portno );
    
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //create file descriptor
     
	if (sockfd < 0)
	{ 
	//	error( const_cast<char *>("ERROR opening socket") );
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr)); //What does this do?

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons( portno );

	//inet_pton(AF_INET, /*"IP address here"*/, &(server_addr.sin_addr) );
	//serverAddr.sin_addr.s_addr = inet_addr(/*"IP address here"*/) ; //Try this if inet_pton() doesn't work

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) //Bind 
	{
	//	error( const_cast<char *>( "ERROR on binding" ) );
	}

	listen(sockfd,5); 


	//clilen = sizeof(cli_addr);
	addr_size = sizeof(serverStorage);
	printf("Listening to port\n");
	newsockfd = accept(sockfd, (struct sockaddr*)&serv_addr, &addr_size);
  
	connectStatus = inSession;

	printf("Entering loop\n");
	//Waiting for connection
	while ( 1 ) //ok, but this isn't the client
	{
		Wifiinterface(); //add this to main code
		//usleep(1500);
	}
	return 0; 
}
