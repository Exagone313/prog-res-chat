#include "protocol.h"

int message_type_to_int(char *msg)
{
	if(msg[0] == 'R' && msg[1] == 'E' && msg[2] == 'G' && msg[3] == 'I' && msg[4] == 'S')
		return 0;
	if(msg[0] == 'W' && msg[1] == 'E' && msg[2] == 'L' && msg[3] == 'C' && msg[4] == 'O')
		return 1;
	if(msg[0] == 'G' && msg[1] == 'O' && msg[2] == 'B' && msg[3] == 'Y' && msg[4] == 'E')
		return 2;
	if(msg[0] == 'C' && msg[1] == 'O' && msg[2] == 'N' && msg[3] == 'N' && msg[4] == 'E')
		return 3;
	if(msg[0] == 'H' && msg[1] == 'E' && msg[2] == 'L' && msg[3] == 'L' && msg[4] == 'O')
		return 4;
	if(msg[0] == 'F' && msg[1] == 'R' && msg[2] == 'I' && msg[3] == 'E' && msg[4] == '?')
		return 5;
	if(msg[0] == 'F' && msg[1] == 'R' && msg[2] == 'I' && msg[3] == 'E' && msg[4] == '>')
		return 6;
	if(msg[0] == 'F' && msg[1] == 'R' && msg[2] == 'I' && msg[3] == 'E' && msg[4] == '<')
		return 7;
	if(msg[0] == 'M' && msg[1] == 'E' && msg[2] == 'S' && msg[3] == 'S' && msg[4] == '?')
		return 8;
	if(msg[0] == 'M' && msg[1] == 'E' && msg[2] == 'N' && msg[3] == 'U' && msg[4] == 'M')
		return 9;
	if(msg[0] == 'M' && msg[1] == 'E' && msg[2] == 'S' && msg[3] == 'S' && msg[4] == '>')
		return 10;
	if(msg[0] == 'M' && msg[1] == 'E' && msg[2] == 'S' && msg[3] == 'S' && msg[4] == '<')
		return 11;
	if(msg[0] == 'F' && msg[1] == 'L' && msg[2] == 'O' && msg[3] == 'O' && msg[4] == '?')
		return 12;
	if(msg[0] == 'F' && msg[1] == 'L' && msg[2] == 'O' && msg[3] == 'O' && msg[4] == '>')
		return 13;
	if(msg[0] == 'L' && msg[1] == 'I' && msg[2] == 'S' && msg[3] == 'T' && msg[4] == '?')
		return 14;
	if(msg[0] == 'R' && msg[1] == 'L' && msg[2] == 'I' && msg[3] == 'S' && msg[4] == 'T')
		return 15;
	if(msg[0] == 'L' && msg[1] == 'I' && msg[2] == 'N' && msg[3] == 'U' && msg[4] == 'M')
		return 16;
	if(msg[0] == 'C' && msg[1] == 'O' && msg[2] == 'N' && msg[3] == 'S' && msg[4] == 'U')
		return 17;
	if(msg[0] == 'S' && msg[1] == 'S' && msg[2] == 'E' && msg[3] == 'M' && msg[4] == '>')
		return 18;
	if(msg[0] == 'M' && msg[1] == 'U' && msg[2] == 'N' && msg[3] == 'E' && msg[4] == 'M')
		return 19;
	if(msg[0] == 'O' && msg[1] == 'O' && msg[2] == 'L' && msg[3] == 'F' && msg[4] == '>')
		return 20;
	if(msg[0] == 'E' && msg[1] == 'I' && msg[2] == 'R' && msg[3] == 'F' && msg[4] == '>')
		return 21;
	if(msg[0] == 'O' && msg[1] == 'K' && msg[2] == 'I' && msg[3] == 'R' && msg[4] == 'F')
		return 22;
	if(msg[0] == 'N' && msg[1] == 'O' && msg[2] == 'K' && msg[3] == 'R' && msg[4] == 'F')
		return 23;
	if(msg[0] == 'A' && msg[1] == 'C' && msg[2] == 'K' && msg[3] == 'R' && msg[4] == 'F')
		return 24;
	if(msg[0] == 'F' && msg[1] == 'R' && msg[2] == 'I' && msg[3] == 'E' && msg[4] == 'N')
		return 25;
	if(msg[0] == 'N' && msg[1] == 'O' && msg[2] == 'F' && msg[3] == 'R' && msg[4] == 'I')
		return 26;
	if(msg[0] == 'L' && msg[1] == 'B' && msg[2] == 'U' && msg[3] == 'P' && msg[4] == '>')
		return 27;
	if(msg[0] == 'N' && msg[1] == 'O' && msg[2] == 'C' && msg[3] == 'O' && msg[4] == 'N')
		return 28;
	if(msg[0] == 'I' && msg[1] == 'Q' && msg[2] == 'U' && msg[3] == 'I' && msg[4] == 'T')
		return 29;
	if(msg[0] == 'P' && msg[1] == 'U' && msg[2] == 'B' && msg[3] == 'L' && msg[4] == '?')
		return 30;
	if(msg[0] == 'P' && msg[1] == 'U' && msg[2] == 'B' && msg[3] == 'L' && msg[4] == '>')
		return 31;
	return 0;
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
