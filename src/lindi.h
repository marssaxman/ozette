#ifndef LINDI_H
#define LINDI_H

#include "ui.h"
#include <string>
#include <list>

class Lindi
{
public:
	Lindi(std::list<std::string> args);
	void run();
private:
	UI _ui;
};

#endif	//LINDI_H
