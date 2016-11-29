#include "dcomm.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>
#include <iostream>

#define DELAY 500
#define MaxByte 1
#define WindowSize 5

//socket
int sockfd;
struct sockaddr_in targetAddr;
unsigned int addrlen = sizeof(targetAddr);
int targetPort;
int recvlen;
char ip[INET_ADDRSTRLEN];

//Deklarasi variabel pendukung
char * filename;
char Message[MAXLEN];
char buff[MaxByte];
char res[MaxByte];
char res1[MaxByte];
int idx = 0;
int MsgLen = 0;
int xon = 1;
int numFrame;
int exit1 = 0;

//Deklarasi prosedur
void readFile (char * filename);
void * threadParent(void *arg);
void * threadChild(void *arg);
void * waitXON(void *arg);
void divideData();

//Array of frane
MESGB *arrayFrame;
MESGB *slidingWindow = (MESGB*) malloc(sizeof(MESGB)*4);
WindowCek *sWindow;
SlidingW *sliding;

int main (int argc, char *argv[]) {
	 Byte c;
	 int transmit = 0;

	// //Buat socket
	 if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	 	perror("cannot create socket");
	 }

	 targetPort = atoi(argv[2]);
	 if (argv[3] != NULL) {
		filename = argv[1];
	 }
	//Baca
	readFile(filename);
	divideData();
	
	memset((char *) &targetAddr, 0 ,sizeof(targetAddr));
	targetAddr.sin_family = AF_INET;
	targetAddr.sin_port = htons(targetPort);
	if (inet_aton((argv[1]), &targetAddr.sin_addr) == 0) {
	 	exit(1);
	} 

	inet_ntop(AF_INET, &(targetAddr.sin_addr), ip, INET_ADDRSTRLEN);


	  //ngenransmit dari file data yg dibaca tadi
	for(int i=0;i<WindowSize;i++) {
		if(sWindow[i].ackStatus==-1 || sWindow[i].ackStatus==1) continue;
		//kirim perwindow, -1=ack		
		//sendto(&arrayFrame[sWIndow[i].frameNum],sizeof(arrayFrame[sWIndow[i].frameNum]));
		sWindow[i].ackStatus = 1; //statusnya kekirim
	}
	
	while(1) //dia ngecek ack
	{
		if(transmit)
		{
			for(int i=0;i<WindowSize;i++) {
				if(sWindow[i].ackStatus==-1 ) continue;
				//sendto(&arrayFrame[sWIndow[i].frameNum],sizeof(arrayFrame[sWIndow[i].frameNum]));
				sWindow[i].ackStatus = 1;
			}
			transmit = 0;
		}
	}


	// readFile(filename);
	// buff[0] = SOH;
	// sendto(sockfd, buff, MaxByte, 0, (struct sockaddr *)&targetAddr, addrlen); //Mengirim data ke receiver
	// printf("Membuat socket untuk koneksi ke %s:%s\n", argv[1], argv[2]);
	// //Thread initialization
	// pthread_t tid[3];
	// int error;
	// error = pthread_create(&(tid[0]), NULL, &threadParent, NULL);
	// if (error!=0) printf("can't create thread : %s\n", strerror(error));

	// error = pthread_create(&(tid[1]), NULL, &threadChild, NULL);
	// if (error!=0) printf("can't create thread : %s\n", strerror(error));

	// error = pthread_create(&(tid[2]), NULL, &waitXON, NULL);
	// if (error!=0) printf("can't create thread : %s\n", strerror(error));

	// pthread_join(tid[0], NULL);
	// pthread_join(tid[1], NULL);
	// pthread_join(tid[2], NULL);
	// close(sockfd);
	// exit(EXIT_SUCCESS);
	for(int i =0; i < 1; i++) {
		// printf("%c\n", arrayFrame[i].data[0]);
	}
}

//Baca file, masukkan ke dalam buffer
void readFile (char * filename) {
	FILE *fp;
	fp = fopen(filename, "r");
	char c; 
	int counter = 0;

	//Read file
	while ((c = fgetc(fp)) != EOF) {
		Message[counter] = c;
		counter++;
	}
	MsgLen = counter; //Panjang file
}

//Thread untuk mengirim data ke receiver
void * threadParent(void *arg) {
	while (true) {
		for (int i=0; i < numFrame; i++) {

		}
	}
	// while (idx < MsgLen) {//Jika index buffer < panjang file
	// 	buff[0] = Message[idx];
	// 	if (xon == 1) { //Belum menerima XOFF
	// 		sendto(sockfd, buff, MaxByte, 0, (struct sockaddr *)&targetAddr, addrlen); //Mengirim data ke receiver
	// 		printf("Mengirim byte ke-%d : '%c'\n", idx+1, Message[idx]);
	// 		idx++;
	// 	}
	// 	usleep(DELAY*5000);
	// }
	// buff[0] = Endfile;
	// sendto(sockfd, buff, MaxByte, 0, (struct sockaddr *)&targetAddr, addrlen);
}

//Thread untuk menerima XON dan XOFF dari receiver
void * threadChild(void *arg) {
	Byte * response = (Byte *)malloc(sizeof(Byte));
	while (idx < MsgLen) {
		recvfrom(sockfd, response, MaxByte, 0, (struct sockaddr *)&targetAddr, &addrlen); //Menerima XON atau XOFF
		if (response[0] == XOFF) {
			xon = 0;
			printf("XOFF dterima...\n");
		}
		else if (response[0] == XON){
			xon = 1;
			printf("XON diterima...\n");
		}
		else {
			break;
		}
	}
}

//Prosedur tambahan untuk menandakan masih menunggu XON diterima
void * waitXON(void *arg) {
	while (idx < MsgLen) {
		while (xon == 0) {
			usleep(DELAY*10000);
			if (xon == 0)
				printf("Menunggu XON...\n");
		}
	}
}

//bagi data menjadi beberapa frame
void divideData() {
	numFrame = MsgLen/6+1;
	int pindahFrame = 0,pindahSlide = 0;
	int q = numFrame/5; //jumlah slidingnya
	if (q*5<numFrame) ++q;
	int numberFrame = -1; //kenapa -1?
		//printf("%d ",  numberFrame);
	int idxData=0;
	arrayFrame = (MESGB *)malloc(sizeof(MESGB)*(MsgLen/6+1));
	sWindow = (WindowCek *)malloc(sizeof(WindowCek)*(MsgLen/6+1));
	sliding = (SlidingW *)malloc(sizeof(SlidingW)*(q));
	unsigned int checksum=0;
	for (int i=0; i < MsgLen; i++) {
		printf("%d\n",i);
		if (i % 6 == 0) {
			if (numberFrame>0) {
				
				arrayFrame[numberFrame-1].checksum = (char) checksum;	
				WindowCek temp = {i,0,arrayFrame[numberFrame-1]};
				sWindow[numberFrame-1] = temp;
				sliding[pindahSlide].all[pindahFrame]=sWindow[numberFrame-1];
				if (pindahFrame<5) {
					pindahFrame++;
				} else {
					pindahFrame = 0;
					pindahSlide++;
				}
			}
			idxData = 0;
			checksum = 0;
			numberFrame++;
			arrayFrame[numberFrame].soh=SOH;

			arrayFrame[numberFrame].stx=STX;
			
			arrayFrame[numberFrame].etx=ETX;
			// printf("masuk\n");
			arrayFrame[numberFrame].magno=(char)numberFrame;
			// printf("masuk2\n");
			arrayFrame[numberFrame].data = (Byte*)malloc(sizeof(Byte)*6) ;
			arrayFrame[numberFrame].data[0]=Message[i];
			// printf("masuk3\n");
			checksum = SOH + STX + ETX + numberFrame + (unsigned char) Message[i];
			
		} else {
			//printf("masuk\n");
			idxData++;
			printf("%c\n",Message[i]);
			arrayFrame[numberFrame].data[idxData] =(Byte) Message[i];
			printf("masuk\n");
			checksum += (unsigned char )Message[i];
		}

		if(i==MsgLen-1){
			arrayFrame[numberFrame].checksum = (char) checksum;
			WindowCek temp = {i,0,arrayFrame[numberFrame]};
			sWindow[numberFrame] = temp;
		}
	}
}

int checkAllAck(SlidingW a) {
	int i = 0;
	while (i<5) {
		if (a.all[i].ackStatus!=1) {
			return 0;		
		}	
	}
	return 1;
}



void* thread1(void *arg) {
	int c=(MsgLen/6+1)/5, i = 0, n = 0;
	if (c*5<(MsgLen/6+1)) ++c;
	while ( i<c ) {

		while ( checkAllAck(sliding[i]) == 0 ) {
			if (sliding[i].all[n].ackStatus!=1) {
				//tambahin kirim
				//sprintf(bufs, "%s", temp.c_str());
				//sendto(sockfd, bufs, MaxFrameLength, 0, (struct sockaddr *)&targetAddr, addrLen);
			}
			if (n<5) {
				n++;
			} else n=0;
		}
		n=0;
		i++;
		usleep((DELAY * 1000)/WindowSize);

	}

	exit1 = 1;
	printf("Exiting1\n");

	return NULL;
}

void* thread2(void *arg) {
	/* Child Thread */
	
	while (exit1 == 0) {
		/* Listening XON XOFF signal */ 
		//ambil recv ack
		//recvfrom(sockfd, res, MaxResponseLength, 0, (struct sockaddr *)&targetAddr, &addrLen)
		//recvfrom(sockfd, res1, MaxResponseLength, 0, (struct sockaddr *)&targetAddr, &addrLen)  
		
		
		if ( (int)res[0] == 6 ) { //ACK
			int u = (int)res1[0]/5;
			if (u*5<(int)res1[0]) ++u;
			sliding[u].all[(int)res1[0] % 5].ackStatus=1;

			printf("ACK");
		} else {
			printf("NAK");	

			
			//kirim lagi dr res1
			//sprintf(bufs, "%s", temp.c_str());
			//(sendto(sockfd, bufs, MaxFrameLength, 0, (struct sockaddr *)&targetAddr, addrLen)	
		}

	}

	printf("Exiting2\n");
	return NULL;
}
