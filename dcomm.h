#ifndef _DCOMM_H_H_
#define _DCOMM_H_H_

/*ASCII Const*/
#define SOH 		1 //Start of header Char
#define STX 		2 //Start of Text Char
#define ETX 		3 //End Of Text Char
#define ENQ 		5 //Enquiry Char
#define ACK 		6 //Acknowledgment
#define BEL 		7 //Message error waring
#define CR 			13 //Carriage Return
#define LF 			10 //Line Feed
#define NAK 		21 //Negatif Acknowledgment
#define Endfile 	26 //End of file char
#define ESC 		27 //ESC key

// XON/XOFF Protocol
#define XON 		(0x11)
#define XOFF		(0x13)

//Const
#define BYTESIZE 	256 //Maximum value of a byte
#define MAXLEN 		1024 //Maximum messages length

typedef unsigned char Byte;

typedef struct QTYPE {
	unsigned int count;
	unsigned int front;
	unsigned int rear;
	unsigned int maxsize;
	Byte *data;
} QTYPE;

typedef struct MESGB {
	unsigned int soh;
	unsigned int stx;
	unsigned int etx;
	Byte checksum;
	Byte magno;
	Byte *data;
} MESGB;

#endif