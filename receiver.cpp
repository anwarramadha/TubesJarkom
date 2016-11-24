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

// Delay to adjust speed of consumming buffer, in milliseconds
#define DELAY 500

//Maximum byte yang dapat diterima
#define MaxByte 1

//Define receive buffer size
#define RXQSIZE 8

//Jumlah buffer yang kosong untuk mengirim XON
#define minimum_upperlimit 6

//Untuk keperluan Socket 
struct sockaddr_in localAddr;
struct sockaddr_in targetAddr;
unsigned int addrlen = sizeof(targetAddr);
int targetPort;
char ip[INET_ADDRSTRLEN];
int sockfd; //listen on sock_fd

//Pengaturan queue sebagai tiruan buffer
Byte rxbuf[RXQSIZE];
QTYPE rcvq = {0, 0, 0, RXQSIZE, rxbuf};
QTYPE * rxq = &rcvq;
Byte result[MAXLEN];
int recvLen;

//bool send_xon = false, send_xoff = false;
int idx = 0;
int recvCounter = 0;
int conCounter = 0;

//Deklarasi variabel pendukung
bool end = false;
bool isXOFF = false;
Byte XONXOFFACK[MaxByte];

//function declaration
static Byte *rcvchar(int sockfd, QTYPE *queue);
void q_get(QTYPE *, Byte *);
void* threadParent(void * arg);
void* threadChild(void * arg);
bool isQueueFull(QTYPE queue);
bool isQueueEmpty(QTYPE queue);
void push(QTYPE * queue, Byte c);
Byte pull(QTYPE * queue);

int main (int argc, char *argv[]) {
	Byte  c;

	//creating socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))< 0) {
		perror("Cannot create socket");
		return 0;
	}

	targetPort = atoi(argv[1]);

	memset((void *)&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &(localAddr.sin_addr));
	localAddr.sin_port = htons(targetPort);
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	//Binding socket
	if (bind(sockfd, (struct sockaddr *)&localAddr, sizeof(localAddr)) <0) {
		perror("Bind error");
		return 0;
	}

	memset((char *) &targetAddr, 0, sizeof(targetAddr));
	targetAddr.sin_family = AF_INET;
	targetAddr.sin_port = htons(targetPort);
	Byte * buf = (Byte *)malloc(sizeof(Byte));
	recvLen = recvfrom(sockfd, buf, MaxByte, 0, (struct sockaddr *)&targetAddr, &addrlen);
	inet_ntop(AF_INET, &(targetAddr.sin_addr), ip, INET_ADDRSTRLEN);
	printf("Binding pada : %s:%d...\n", ip, targetPort);

	//Buat thread untuk 2 prosedur
	pthread_t tid[2];
	int error;
	error = pthread_create(&(tid[0]), NULL, &threadParent, NULL);
	if (error!=0) printf("can't create thread : %s\n", strerror(error));

	error = pthread_create(&(tid[1]), NULL, &threadChild, NULL);
	if (error!=0) printf("can't create thread : %s\n", strerror(error));

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);

	//cetak hasil yang diterima
	printf("\n\n%s\n", result);
}

//Menerima karakter dari transmitter kemudian memasukkan ke queue
static Byte *rcvchar (int sockfd, QTYPE *queue) {
	Byte * buf = (Byte *)malloc(sizeof(Byte));
	recvLen = recvfrom(sockfd, buf, MaxByte, 0, (struct sockaddr *)&targetAddr, &addrlen);
	if (!isQueueFull(*queue) && buf[0] != Endfile) { //Karakter yang diterima bukan Endfile
		push(queue, buf[0]); //Memasukkan karakter ke dalam queue
	}
	if (buf[0] == Endfile) end = true; //Karakter yang diterima berupa Endfile
	return buf;
}

//Memasukkan nilai dari queue (buffer tiruan) ke dalam array of char (I/O tiruan)
void q_get(QTYPE *queue, Byte *data) {
	char current;
	if (isQueueEmpty(*queue)){}
	else {
		usleep(DELAY*15000);
		current = pull(&*queue);
		printf("Mengkonsumsi byte ke-%d: '%c'\n", conCounter+1, current);
		data[idx] = current;
		idx++;
		conCounter++;
	}
}

//Thread parent sebagai thread utama untuk menghandle masuknya data dan mengirim XOFF jika buffer penuh
void* threadParent(void * arg) {
	Byte * c = (Byte *)malloc(sizeof(Byte));
	while (!end) { //Bukan Endfile
		if (!isQueueFull(*rxq)) { //queue / buffer belum penuh
			c = rcvchar(sockfd, rxq);
			if (c != NULL) {
				printf("Menerima byte ke-%d\n", recvCounter+1);
				recvCounter++;
			}
		}
		else { //queue penuh
			if (!isXOFF) {
				printf("Buffer > Minimum upperlimit\n");
				if (isQueueFull(*rxq)) {
					XONXOFFACK[0] = XOFF;
					printf("Mengirim XOFF...\n");
					if (sendto(sockfd, XONXOFFACK, MaxByte, 0, (struct sockaddr *)&targetAddr, addrlen) == -1) { //Kirim XOFF ke transmitter
						perror("err");
					}
					isXOFF = true;
				}
			}
		}
	}
	XONXOFFACK[0] = ACK;
	sendto(sockfd, XONXOFFACK, MaxByte, 0, (struct sockaddr *)&targetAddr, addrlen);
}

//Thread child digunakan sebagai pengonsumsi karakter dari buffer ke I/O dan mengirimkan XON ke transmitter jika di buffer ada space > minimum upperlimit
void * threadChild(void * arg) {
	while (!isQueueEmpty(*rxq) || !end) { //Buffer tidak kosong atau bukan akhir dari file
		q_get(&*rxq, result); //masukkan karakter dari buffer ke I/O
		if (rxq->count < minimum_upperlimit && isXOFF) { //Jika ada space kosong di buffer > minimum upperlimit, maka kirim XON
			printf("Buffer < maximum lowerlimit\n");
			XONXOFFACK[0] = XON;
			printf("Mengirim XON...\n");
			if (sendto(sockfd, XONXOFFACK, MaxByte, 0, (struct sockaddr *)&targetAddr, addrlen) == -1) { //kirim XON

			}
			isXOFF = false;
		}
	}
}

//Queue Penuh
bool isQueueFull(QTYPE queue) {
	return queue.count == queue.maxsize;
}

//Queue Kosong
bool isQueueEmpty(QTYPE queue) {
	return queue.count == 0;
}

//Masukkan data ke queue
void push(QTYPE * queue, Byte data) {
	queue->data[queue->rear] = data;
	queue->rear++;
	queue->count++;
	if (queue->rear == queue->maxsize) queue->rear = 0;
}

//Ambil data dari queue
Byte pull(QTYPE * queue) {
	Byte c;
	if (queue->front == queue->maxsize) queue->front = 0;
	c = queue->data[queue->front];
	queue->front++;
	queue->count--;
	return c;
}

bool checkchecksum(MESGB inp) {
	unsigned int temp=inp.soh+inp.stx +inp.etx+(unsigned char)inp.magno;
	for (int i = 0; i < inp.data.length; i++) {
		temp+=(unsigned char)inp.data[i];
	}
	if (temp == (unsigned char)inp.checksum) return true;
	return false;
}