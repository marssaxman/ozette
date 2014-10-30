#include "ui.h"

int main()
{
	UI ui;
	do {
		update_panels();
		doupdate();
	} while (ui.process(getch()));
	return 0;
}
