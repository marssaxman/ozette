#ifndef BUFFER_H
#define BUFFER_H

#include <ncurses.h>
#include <string>
#include <vector>

class Buffer
{
public:
	void layout(WINDOW *dest, int yoff, int xoff, int height, int width);
	void set_focus();
	void fill(std::string text);
protected:
	void position_cursor();
	void blitline(int index, std::string line);
private:
	WINDOW *_window = nullptr;
	int _yoff = 0;
	int _xoff = 0;
	int _width = 0;
	int _height = 0;
	int _xcursor = 0;
	int _ycursor = 0;
};

#endif // CONTROLLER_H
