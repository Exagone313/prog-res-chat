#include "protocol.h"

int message_type_to_int(char *msg)
{
	if(msg[0] == 'R' && msg[1] == 'E' && msg[2] == 'G' && msg[3] == 'I' && msg[4] == 'S')
		return REGIS;
	if(msg[0] == 'W' && msg[1] == 'E' && msg[2] == 'L' && msg[3] == 'C' && msg[4] == 'O')
		return WELCO;
	if(msg[0] == 'G' && msg[1] == 'O' && msg[2] == 'B' && msg[3] == 'Y' && msg[4] == 'E')
		return GOBYE;
	if(msg[0] == 'C' && msg[1] == 'O' && msg[2] == 'N' && msg[3] == 'N' && msg[4] == 'E')
		return CONNE;
	if(msg[0] == 'H' && msg[1] == 'E' && msg[2] == 'L' && msg[3] == 'L' && msg[4] == 'O')
		return HELLO;
	if(msg[0] == 'F' && msg[1] == 'R' && msg[2] == 'I' && msg[3] == 'E' && msg[4] == '?')
		return FRIEX;
	if(msg[0] == 'F' && msg[1] == 'R' && msg[2] == 'I' && msg[3] == 'E' && msg[4] == '>')
		return FRIEY;
	if(msg[0] == 'F' && msg[1] == 'R' && msg[2] == 'I' && msg[3] == 'E' && msg[4] == '<')
		return FRIEZ;
	if(msg[0] == 'M' && msg[1] == 'E' && msg[2] == 'S' && msg[3] == 'S' && msg[4] == '?')
		return MESSX;
	if(msg[0] == 'M' && msg[1] == 'E' && msg[2] == 'N' && msg[3] == 'U' && msg[4] == 'M')
		return MENUM;
	if(msg[0] == 'M' && msg[1] == 'E' && msg[2] == 'S' && msg[3] == 'S' && msg[4] == '>')
		return MESSY;
	if(msg[0] == 'M' && msg[1] == 'E' && msg[2] == 'S' && msg[3] == 'S' && msg[4] == '<')
		return MESSZ;
	if(msg[0] == 'F' && msg[1] == 'L' && msg[2] == 'O' && msg[3] == 'O' && msg[4] == '?')
		return FLOOX;
	if(msg[0] == 'F' && msg[1] == 'L' && msg[2] == 'O' && msg[3] == 'O' && msg[4] == '>')
		return FLOOY;
	if(msg[0] == 'L' && msg[1] == 'I' && msg[2] == 'S' && msg[3] == 'T' && msg[4] == '?')
		return LISTX;
	if(msg[0] == 'R' && msg[1] == 'L' && msg[2] == 'I' && msg[3] == 'S' && msg[4] == 'T')
		return RLIST;
	if(msg[0] == 'L' && msg[1] == 'I' && msg[2] == 'N' && msg[3] == 'U' && msg[4] == 'M')
		return LINUM;
	if(msg[0] == 'C' && msg[1] == 'O' && msg[2] == 'N' && msg[3] == 'S' && msg[4] == 'U')
		return CONSU;
	if(msg[0] == 'S' && msg[1] == 'S' && msg[2] == 'E' && msg[3] == 'M' && msg[4] == '>')
		return SSEMY;
	if(msg[0] == 'M' && msg[1] == 'U' && msg[2] == 'N' && msg[3] == 'E' && msg[4] == 'M')
		return MUNEM;
	if(msg[0] == 'O' && msg[1] == 'O' && msg[2] == 'L' && msg[3] == 'F' && msg[4] == '>')
		return OOLFY;
	if(msg[0] == 'E' && msg[1] == 'I' && msg[2] == 'R' && msg[3] == 'F' && msg[4] == '>')
		return EIRFY;
	if(msg[0] == 'O' && msg[1] == 'K' && msg[2] == 'I' && msg[3] == 'R' && msg[4] == 'F')
		return OKIRF;
	if(msg[0] == 'N' && msg[1] == 'O' && msg[2] == 'K' && msg[3] == 'R' && msg[4] == 'F')
		return NOKRF;
	if(msg[0] == 'A' && msg[1] == 'C' && msg[2] == 'K' && msg[3] == 'R' && msg[4] == 'F')
		return ACKRF;
	if(msg[0] == 'F' && msg[1] == 'R' && msg[2] == 'I' && msg[3] == 'E' && msg[4] == 'N')
		return FRIEN;
	if(msg[0] == 'N' && msg[1] == 'O' && msg[2] == 'F' && msg[3] == 'R' && msg[4] == 'I')
		return NOFRI;
	if(msg[0] == 'L' && msg[1] == 'B' && msg[2] == 'U' && msg[3] == 'P' && msg[4] == '>')
		return LBUPY;
	if(msg[0] == 'N' && msg[1] == 'O' && msg[2] == 'C' && msg[3] == 'O' && msg[4] == 'N')
		return NOCON;
	if(msg[0] == 'I' && msg[1] == 'Q' && msg[2] == 'U' && msg[3] == 'I' && msg[4] == 'T')
		return IQUIT;
	if(msg[0] == 'P' && msg[1] == 'U' && msg[2] == 'B' && msg[3] == 'L' && msg[4] == '?')
		return PUBLX;
	if(msg[0] == 'P' && msg[1] == 'U' && msg[2] == 'B' && msg[3] == 'L' && msg[4] == '>')
		return PUBLY;
	return -1;
}

void int_to_message_type(int code, char *msg)
{
	if(code == REGIS) {
		msg[0] = 'R';
		msg[1] = 'E';
		msg[2] = 'G';
		msg[3] = 'I';
		msg[4] = 'S';
		return;
	}
	if(code == WELCO) {
		msg[0] = 'W';
		msg[1] = 'E';
		msg[2] = 'L';
		msg[3] = 'C';
		msg[4] = 'O';
		return;
	}
	if(code == GOBYE) {
		msg[0] = 'G';
		msg[1] = 'O';
		msg[2] = 'B';
		msg[3] = 'Y';
		msg[4] = 'E';
		return;
	}
	if(code == CONNE) {
		msg[0] = 'C';
		msg[1] = 'O';
		msg[2] = 'N';
		msg[3] = 'N';
		msg[4] = 'E';
		return;
	}
	if(code == HELLO) {
		msg[0] = 'H';
		msg[1] = 'E';
		msg[2] = 'L';
		msg[3] = 'L';
		msg[4] = 'O';
		return;
	}
	if(code == FRIEX) {
		msg[0] = 'F';
		msg[1] = 'R';
		msg[2] = 'I';
		msg[3] = 'E';
		msg[4] = '?';
		return;
	}
	if(code == FRIEY) {
		msg[0] = 'F';
		msg[1] = 'R';
		msg[2] = 'I';
		msg[3] = 'E';
		msg[4] = '>';
		return;
	}
	if(code == FRIEZ) {
		msg[0] = 'F';
		msg[1] = 'R';
		msg[2] = 'I';
		msg[3] = 'E';
		msg[4] = '<';
		return;
	}
	if(code == MESSX) {
		msg[0] = 'M';
		msg[1] = 'E';
		msg[2] = 'S';
		msg[3] = 'S';
		msg[4] = '?';
		return;
	}
	if(code == MENUM) {
		msg[0] = 'M';
		msg[1] = 'E';
		msg[2] = 'N';
		msg[3] = 'U';
		msg[4] = 'M';
		return;
	}
	if(code == MESSY) {
		msg[0] = 'M';
		msg[1] = 'E';
		msg[2] = 'S';
		msg[3] = 'S';
		msg[4] = '>';
		return;
	}
	if(code == MESSZ) {
		msg[0] = 'M';
		msg[1] = 'E';
		msg[2] = 'S';
		msg[3] = 'S';
		msg[4] = '<';
		return;
	}
	if(code == FLOOX) {
		msg[0] = 'F';
		msg[1] = 'L';
		msg[2] = 'O';
		msg[3] = 'O';
		msg[4] = '?';
		return;
	}
	if(code == FLOOY) {
		msg[0] = 'F';
		msg[1] = 'L';
		msg[2] = 'O';
		msg[3] = 'O';
		msg[4] = '>';
		return;
	}
	if(code == LISTX) {
		msg[0] = 'L';
		msg[1] = 'I';
		msg[2] = 'S';
		msg[3] = 'T';
		msg[4] = '?';
		return;
	}
	if(code == RLIST) {
		msg[0] = 'R';
		msg[1] = 'L';
		msg[2] = 'I';
		msg[3] = 'S';
		msg[4] = 'T';
		return;
	}
	if(code == LINUM) {
		msg[0] = 'L';
		msg[1] = 'I';
		msg[2] = 'N';
		msg[3] = 'U';
		msg[4] = 'M';
		return;
	}
	if(code == CONSU) {
		msg[0] = 'C';
		msg[1] = 'O';
		msg[2] = 'N';
		msg[3] = 'S';
		msg[4] = 'U';
		return;
	}
	if(code == SSEMY) {
		msg[0] = 'S';
		msg[1] = 'S';
		msg[2] = 'E';
		msg[3] = 'M';
		msg[4] = '>';
		return;
	}
	if(code == MUNEM) {
		msg[0] = 'M';
		msg[1] = 'U';
		msg[2] = 'N';
		msg[3] = 'E';
		msg[4] = 'M';
		return;
	}
	if(code == OOLFY) {
		msg[0] = 'O';
		msg[1] = 'O';
		msg[2] = 'L';
		msg[3] = 'F';
		msg[4] = '>';
		return;
	}
	if(code == EIRFY) {
		msg[0] = 'E';
		msg[1] = 'I';
		msg[2] = 'R';
		msg[3] = 'F';
		msg[4] = '>';
		return;
	}
	if(code == OKIRF) {
		msg[0] = 'O';
		msg[1] = 'K';
		msg[2] = 'I';
		msg[3] = 'R';
		msg[4] = 'F';
		return;
	}
	if(code == NOKRF) {
		msg[0] = 'N';
		msg[1] = 'O';
		msg[2] = 'K';
		msg[3] = 'R';
		msg[4] = 'F';
		return;
	}
	if(code == ACKRF) {
		msg[0] = 'A';
		msg[1] = 'C';
		msg[2] = 'K';
		msg[3] = 'R';
		msg[4] = 'F';
		return;
	}
	if(code == FRIEN) {
		msg[0] = 'F';
		msg[1] = 'R';
		msg[2] = 'I';
		msg[3] = 'E';
		msg[4] = 'N';
		return;
	}
	if(code == NOFRI) {
		msg[0] = 'N';
		msg[1] = 'O';
		msg[2] = 'F';
		msg[3] = 'R';
		msg[4] = 'I';
		return;
	}
	if(code == LBUPY) {
		msg[0] = 'L';
		msg[1] = 'B';
		msg[2] = 'U';
		msg[3] = 'P';
		msg[4] = '>';
		return;
	}
	if(code == NOCON) {
		msg[0] = 'N';
		msg[1] = 'O';
		msg[2] = 'C';
		msg[3] = 'O';
		msg[4] = 'N';
		return;
	}
	if(code == IQUIT) {
		msg[0] = 'I';
		msg[1] = 'Q';
		msg[2] = 'U';
		msg[3] = 'I';
		msg[4] = 'T';
		return;
	}
	if(code == PUBLX) {
		msg[0] = 'P';
		msg[1] = 'U';
		msg[2] = 'B';
		msg[3] = 'L';
		msg[4] = '?';
		return;
	}
	if(code == PUBLY) {
		msg[0] = 'P';
		msg[1] = 'U';
		msg[2] = 'B';
		msg[3] = 'L';
		msg[4] = '>';
		return;
	}
}
