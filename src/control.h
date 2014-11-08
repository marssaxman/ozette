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
	// Dedicated keys for editor manipulation
	Tab = 0x09, // HT ^I
	Enter = 0x0A, //LF ^J
	Return = 0x0D, //CR ^M
	Escape = 0x1B, // ESC ^[
	Backspace = 0x7F, // DEL

	Cut = 0x18, //CAN ^X
	Copy = 0x03, //ETX ^C
	Paste = 0x16, //SYN ^V
	Undo = 0x1A, //SUB ^Z
	Redo = 0x19, //EM ^Y

	Open = 0x0F, //SI ^O
	Close = 0x17, //ETB ^W
	NewFile = 0x0E, // SO ^N
	Save = 0x13, //DC3 ^S

	Find = 0x06, //ACK ^F
	GoTo = 0x07, //BEL ^G

	Directory = 0x04, //EOT ^D
	Quit = 0x11, //DC1 ^Q
	Help = 0x1F, //US ^?

	// Ideas for the future
	// R = run, B = build, K = clean?
	// edit, diff, log, blame?
};

struct Panel {
	static const size_t width = 6;
	static const size_t height = 2;
	char label[height][width];
};

} // namespace Control

#endif	//CONTROL_H
