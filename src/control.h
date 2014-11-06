#ifndef CONTROL_H
#define CONTROL_H

#include <stddef.h>

namespace Control {
static const unsigned Count = 32;
struct Info {
	char mnemonic;
	char label[10];
};
extern Info keys[Count];

enum {
	Cut = 0x18, //CAN ^X
	Copy = 0x03, //ETX ^C
	Paste = 0x16, //SYN ^V
	Delete = 0x0B, //VT ^K
	Undo = 0x1A, //SUB ^Z

	Open = 0x0F, //SI ^O
	Close = 0x17, //ETB ^W
	Save = 0x13, //DC3 ^S
	Revert = 0x12, //DC2 ^R

	Find = 0x06, //ACK ^F
	GoTo = 0x07, //BEL ^G

	Quit = 0x11, //DC1 ^Q
};

struct Panel {
	static const size_t width = 6;
	static const size_t height = 2;
	char label[height][width];
};

} // namespace Control

#endif	//CONTROL_H
