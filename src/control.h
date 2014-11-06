#ifndef CONTROL_H
#define CONTROL_H

#include <stddef.h>

namespace Control {
static const unsigned Count = 32;
struct Info {
	char key;
	char label[10];
};
extern Info keys[Count];

enum {
	Cut = 0x18, //^X
	Copy = 0x03, //^C
	Paste = 0x16, //^V

	Quit = 0x11, //^Q
};

struct Panel {
	static const size_t width = 6;
	static const size_t height = 2;
	char label[height][width];
};

} // namespace Control

#endif	//CONTROL_H
