#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <string>
#include "buffer.h"

class Controller
{
public:
	virtual ~Controller() = default;
	virtual void init(Buffer &buffer) = 0;
	// return true to stay alive, false to terminate
	virtual bool process(Buffer &buffer, int ch) = 0;
	virtual std::string title() const = 0;
};

#endif // CONTROLLER_H
