#ifndef PROTOCOL_H
#define PROTOCOL_H

// ? => X, > => Y, < => Z
#define REGIS 0
#define WELCO 1
#define GOBYE 2
#define CONNE 3
#define HELLO 4
#define FRIEX 5
#define FRIEY 6
#define FRIEZ 7
#define MESSX 8
#define MENUM 9
#define MESSY 10
#define MESSZ 11
#define FLOOX 12
#define FLOOY 13
#define LISTX 14
#define RLIST 15
#define LINUM 16
#define CONSU 17
#define SSEMY 18
#define MUNEM 19
#define OOLFY 20
#define EIRFY 21
#define OKIRF 22
#define NOKRF 23
#define ACKRF 24
#define FRIEN 25
#define NOFRI 26
#define LBUPY 27
#define NOCON 28
#define IQUIT 29
#define PUBLX 30
#define PUBLY 31

int message_type_to_int(char *msg); // reads the first 5 bytes that must be set
void int_to_message_type(int code, char *msg); // sets the first 5 bytes that must be set

#endif
