//Final version that will use the networking code. 

//Header file to use process functions and pipes
#include <unistd.h>
//Header file to use exit() function and streams
#include <stdlib.h>
//Header file to define values for errno
#include <errno.h>
//Header file for wiringPi GPIO programing functions
#include <wiringPi.h>
//Header file for streams
#include <stdio.h>
//Header file for the timer
#include <time.h>
//Header files needed for networking functions
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
//Header file for manipulating strings
#include <string.h>
//Accelerometer header files
#include <wiringPiI2C.h>
//Used for int16_t
#include <stdint.h>
//Used to print int16_t
#include <inttypes.h>

//////////////////Function Prototypes/////////////////////
pid_t playHit();
pid_t playMiss();
void killSong(pid_t pid);
char getData(int sockfd);
int checkSession();
void startServer();
void error(char *msg);
//accelerometer function prototypes
struct acc_dat;
void adx1345_init(int fd);
struct acc_dat adx1345_read_xyz(int fd);
struct acc_dat rawDataToAcceleration(struct acc_dat a);
int low;		//low signal from accelerometer
int medium;		//medium signal from accelerometer
int high;		//high signal from accelerometer

/////////////////////Interrupt Handler/////////////////////
void setSoccer();

///////////////////////Variables/////////////////////////
struct timespec timeOne, timeTwo; //timers
char hit = 0x00;
char miss = 0x01;
char cutoff = 0x05; //Used for shutdown command
char turnoff = 0x06;	//Used for killing this program
pid_t id;
int sockfd, newsockfd;
char data;
struct sockaddr_in serv_addr;
struct sockaddr_storage serverStorage; 
socklen_t addr_size;
char flag = 0x01;	//Used to switch between inSession and restablish
int fd;			//file descriptor used for accelerometer
struct acc_dat acc_xyz;	//Struct for accelerometer coordinates


//This is the I2C address
#define DevAddr 0x53 //device address

////////////////////accelerometer coordinate struct/////////////
struct acc_dat
{	
	//double x;	
	//double y;
	//double z;

	//int x;	
	//int y;
	//int z;

	signed int x;
	signed int y;
	signed int z;

	//int16_t x;
	//int16_t y;
	//int16_t z;
};

////////////////////enum for Wifi tick function/////////////
enum Connection {inSession, restablish} connectStatus;

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

		//Check if shutdown signal came
		if(data == cutoff)
		{
			system("shutdown -h now");
		}

		//Check if turnoff signal came
		if(data == turnoff)
		{
			close(newsockfd);
			close(sockfd);
			exit(0);
		}
		
	
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

int main()
{
///////////////////////////setup accelerometer//////////////////////////////////
	fd = wiringPiI2CSetup(DevAddr);

	if(-1 == fd)
	{
		perror("I2C device setup error");
	}

	adx1345_init(fd);


/////////////////////////////setup audio/////////////////////////////////////
	//set up timeOne
	clock_gettime(CLOCK_MONOTONIC, &timeOne);
	
	//wiringPi setup
	//Sets up program to use RPi with pins
	wiringPiSetup();
	pinMode(0, INPUT);
	pullUpDnControl(0, PUD_DOWN);
	wiringPiISR(0, INT_EDGE_RISING, &setSoccer);


/////////////////////////////setup wifi///////////////////////////////////////
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

/////////////////////////Audio functions//////////////////////////////

//Soccer process uses this as a callback to place value from pin 11 in the stream
void setSoccer()
{
	clock_gettime(CLOCK_MONOTONIC, &timeTwo);
	
	double difference = difftime(timeTwo.tv_sec, timeOne.tv_sec);

	//Song needs to be longer than a second to kill and replay properly.
	if(difference > 1)
	{
		//accelerometer read
		acc_xyz = adx1345_read_xyz(fd);
		acc_xyz = rawDataToAcceleration(acc_xyz);
	
		//if(acc_xyz.y < low)
		//	printf("Low");
		//if(acc_xyz.y < medium)
		//	printf("medium");
		//if(acc_xyz.y < high)
		//	printf("high");

		printf("x: %05d  y: %05d   z: %05d\n", acc_xyz.x, acc_xyz. y-9, acc_xyz.z);	

		killSong(id);
		
		if(data == hit)
			id = playHit();
		if(data == miss)
			id = playMiss();

		clock_gettime(CLOCK_MONOTONIC, &timeOne);
	}
	
	return;
}

//Used to play hit song and return process ID. ID not really necessary, just kill omxplayer.bin
pid_t playHit()
{
	pid_t temp = fork();
	if(temp == 0) //child
	{
		printf("hit\n");
		
		execlp("/usr/bin/omxplayer", "omxplayer", "-o", "local", "/home/pi/SCFS/hit.mp3", NULL);
		//execlp("/usr/bin/omxplayer", "omxplayer", "-o", "hdmi", "/home/pi/SCFS/hit.mp3", NULL);
		//kill(id, 9); //suicide. Doesn't really work
	}
	else	//parent
	{
		return temp;
	}
}

//Used to play miss song and return process ID. ID not really necessary, just kill omxplayer.bin
pid_t playMiss()
{
	pid_t temp = fork();
	if(temp == 0) //child
	{
		printf("hit\n");
		
		execlp("/usr/bin/omxplayer", "omxplayer", "-o", "local", "/home/pi/SCFS/miss.mp3", NULL);
		//execlp("/usr/bin/omxplayer", "omxplayer", "-o", "hdmi", "/home/pi/SCFS/miss.mp3", NULL);
		//kill(id, 9); //suicide. Doesn't really work
	}
	else	//parent
	{
		return temp;
	}
}

//Used to kill omxplayer.bin. Killing child processes doesn't stop omxplayer
void killSong(pid_t pid)
{
	printf("In kill function\n");
	system("killall omxplayer.bin");
	return;
}




//////////////////////////////Networking functions///////////////////////////
//Makes a new soccet for app to reconnect
void startServer()
{
	close(newsockfd);
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

//////////////////////////////////////////////Accelerometer functions////////////////////////////////////

//This function is used to initialize the accelerometers registers
void adx1345_init(int fd)
{
	//These functions are used to write into an 8 bit register
	//wiringPiI2CwriteReg8( fileDescriptor, registor, data)
	//Sets up data representation, interrupts, sleep mode settings, and data offsets
	//Reg 0x31 = DATA_FORMAT	controls representation of data from axis (x, y, z), regs 32~37. D1 and D0 determines 2g, 4g, 8g, or 16g
	//Reg 0x2d = POWER_CTL		Power optimization via sleep, wakeup, etc
	//Reg 0x2e = INT_ENABLE		Setting bits to 1 allow mapped functions (data ready, single tap, double, activity, etc) to use interrupts
	//Reg 0x1e = OFSX		offer user-set offset adjustments in twos complement format with a scale of 15.6mg/LSB (0x7F = +2g)
	//Reg 0x1f = OFSY
	//Reg 0x20 = OFSX
	wiringPiI2CWriteReg8(fd, 0x31, 0x0b);	//D1 D0 = 1 1 => 16g: 31.2mg/LSB.    D3 = 1: full resolution mode
	wiringPiI2CWriteReg8(fd, 0x2d, 0x08);
	wiringPiI2CWriteReg8(fd, 0x2e, 0x00);
	wiringPiI2CWriteReg8(fd, 0x1e, 0x00);
	wiringPiI2CWriteReg8(fd, 0x1f, 0x00);
	wiringPiI2CWriteReg8(fd, 0x20, 0x00);


	//Sets up time for interrupt between taps
	//Reg 0x21 = DUR	contains unsigned time value representing maximum time that an event must be above THRESH_TAP to qualify as tap event
	//Reg 0x22 = Latent	contains unsigned time value representing wait time from detection of tap event to start of time window
	//Reg 0x23 = Window	contains unsigned time value representing time after expiration of latecy for valid second tap
	wiringPiI2CWriteReg8(fd, 0x21, 0x00);
	wiringPiI2CWriteReg8(fd, 0x22, 0x00);
	wiringPiI2CWriteReg8(fd, 0x23, 0x00);
	

	//Sets up details for activity detection and DC or AC data output 
	//Reg 0x24 = THRESH_ACT		holds threshold value for detecting activity. 
	//Reg 0x25 = THRESH_INACT	holds threshold value for detecting inactivity
	//Reg 0x26 = TIME_INACT		contains unsigned time value representing time that acceleration must less than THRES_INACT for inactivity to be declared
	//Reg 0x27 = ACT_INACT_CTL	bit mapped for DC or AC acceleration. 0 for DC, 1 for AC
	wiringPiI2CWriteReg8(fd, 0x24, 0x01);
	wiringPiI2CWriteReg8(fd, 0x25, 0x0f);
	wiringPiI2CWriteReg8(fd, 0x26, 0x2b);
	wiringPiI2CWriteReg8(fd, 0x27, 0x00);


	//Info for freefall and FIFO register
	//Reg 0x28 = THRESH_FF		holds unsigned threshold value for freefall. RSS of all axis used to determine this
	//Rg 0x29 = TIME_FF		unsigned time value representing minimum time RSS has to be 
	//Reg 0x2a = TAP_AXES		Enables x, y, or z axis for tap functionality
	//Reg 0x2c = BW_RATE		Sets power mode and output data rate
	//Reg 0x2f = INT_MAP		Determines where interrupts are sent for bit mapped functions. 0 for INT1 pin, 1 for INT2 pin
	//Reg 0x38 = FIFO_CTL
	wiringPiI2CWriteReg8(fd, 0x28, 0x09);
	wiringPiI2CWriteReg8(fd, 0x29, 0xff);
	wiringPiI2CWriteReg8(fd, 0x2a, 0x80);
	wiringPiI2CWriteReg8(fd, 0x2c, 0x0a);
	wiringPiI2CWriteReg8(fd, 0x2f, 0x00);
	wiringPiI2CWriteReg8(fd, 0x38, 0x9f);
}

//This function gets the output from the accelerometer
struct acc_dat adx1345_read_xyz(int fd)
{
	char x0, y0, z0, x1, y1, z1;
	//char is already 8 bits, so...
	struct acc_dat acc_xyz;

	//Reg 0x32 = DATAX0 (least significant byte)
	//Reg 0x33 = DATAX1 (most significant byte)
	//Reg 0x34 = DATAY0
	//Reg 0x35 = DATAY1
	//Reg 0x36 = DATAZ0
	//Reg 0x37 = DATAZ1
	x0 = 0xff - wiringPiI2CReadReg8(fd, 0x32); //Two's compliment => Unsigned value: (!Nr = rn - N) => (N = rn - !Nr)
	x1 = 0xff - wiringPiI2CReadReg8(fd, 0x33);
	y0 = 0xff - wiringPiI2CReadReg8(fd, 0x34);
	y1 = 0xff - wiringPiI2CReadReg8(fd, 0x35);
	z0 = 0xff - wiringPiI2CReadReg8(fd, 0x36);
	z1 = 0xff - wiringPiI2CReadReg8(fd, 0x37);
	
	//combining both 8 bit register values into a single 16bit value. Char is 8 bits though, so....
	acc_xyz.x = (int)(x1 << 8) + (int)x0;
	acc_xyz.y = (int)(y1 << 8) + (int)y0;
	acc_xyz.z = (int)(z1 << 8) + (int)z0;

	//acc_xyz.x = (x1 << 8) | x0;
	//acc_xyz.y = (y1 << 8) | y0;
	//acc_xyz.z = (z1 << 8) | z0;

	//acc_xyz.x = (double)acc_xyz.x;
	//acc_xyz.y = (double)acc_xyz.y;
	//acc_xyz.z = (double)acc_xyz.z;

	//acc_xyz.x = (signed int)acc_xyz.x;
	//acc_xyz.y = (signed int)acc_xyz.y;
	//acc_xyz.z = (signed int)acc_xyz.z;
	
	//acc_xyz.x = (int16_t)acc_xyz.x;
	//acc_xyz.y = (int16_t)acc_xyz.y;
	//acc_xyz.z = (int16_t)acc_xyz.z;

	return acc_xyz;
}

struct acc_dat rawDataToAcceleration(struct acc_dat a)
{
	//Temp struct to return
	struct acc_dat temp;
	temp.x = a.x;
	temp.y = a.y;
	temp.z = a.z;
	
	//Multiply the raw data by the mg/LSB to get the acceleration *in g*
	//16 bit mode and full resolution => 3.9mG/LSB ?
	//temp.x = (temp.x * 3.9)/1000; //Approximated. Should be 31.2. Change from int to double?
	//temp.y = (temp.y * 3.9)/1000;
	//temp.z = (temp.z * 3.9)/1000;

	////Multiply (9.8/1g) to get data into m/s^2
	//temp.x = temp.x * 9.8;	
	//temp.y = temp.y * 9.8;
	//temp.z = temp.z * 9.8;

	//temp.x = (temp.x - 127.5)/(-126.5);
	//temp.y = (temp.y - 118)/(-127);/home/
	//temp.z = (temp.z - 127)/(-127);

	//temp.x = (temp.x - 32776.55)/(32521.925);
	//temp.y = (temp.y - 32724.15)/(32481.4);
	//temp.z = (temp.z - 32757.75)/(32521.25);

	return temp;
}