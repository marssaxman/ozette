#ifndef CONTROL_H
#define CONTROL_H

#include <stddef.h>

namespace Control {
enum {
	// Dedicated keys for editor manipulation
	Tab = 0x09, // HT ^I
	Enter = 0x0A, //LF ^J
	Return = 0x0D, //CR ^M
	Escape = 0x1B, // ESC ^[
	Backspace = 0x7F, // DEL
	LeftArrow = 0x21C,
	RightArrow = 0x22B,
	UpArrow = 0x231,
	DownArrow = 0x208,
	ShiftLeftArrow = 0x21D,
	ShiftRightArrow = 0x22C,

	// Clipboard & editing functions
	Cut = 0x18, //CAN ^X
	Copy = 0x03, //ETX ^C
	Paste = 0x16, //SYN ^V
	Undo = 0x1A, //SUB ^Z
	Redo = 0x19, //EM ^Y

	// File management
	Open = 0x0F, //SI ^O
	Close = 0x17, //ETB ^W
	NewFile = 0x0E, // SO ^N
	Save = 0x13, //DC3 ^S

	// Navigation
	Find = 0x06, //ACK ^F
	FindNext = 0x07, // BEL ^G
	ToLine = 0x0C, //FF ^L

	// Application global commands
	Directory = 0x04, //EOT ^D
	Quit = 0x11, //DC1 ^Q
	Help = 0x1F, //US ^?

	// Unused ASCII control codes
	SOH = 0x01, // ! A a
	STX = 0x02, // " B b
	ENQ = 0x05, // % E e
	BS = 0x08, // ( H h
	VT = 0x0B, // + K k
	DLE = 0x10, // 0 P p
	DC2 = 0x12, // 2 R r
	DC4 = 0x14, // 4 T t
	NAK = 0x15, // 5 U u
	FS = 0x1C, // < \ |
	GS = 0x1D, // = ] }
	RS = 0x1E, // > ^ ~
};
} // namespace Control

#endif	//CONTROL_H
