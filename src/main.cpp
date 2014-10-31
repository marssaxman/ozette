#include "ui.h"
#include "console.h"
#include "browser.h"
#include "editor.h"

int main(int argc, char **argv)
{
	UI ui;
        // Build our two starting windows.
        std::unique_ptr<Controller> console(new Console);
        ui.open_window(std::move(console));
        std::unique_ptr<Controller> browser(new Browser);
        ui.open_window(std::move(browser));
	// If we got a list of arguments, create editors for them.
	for (int i = 1; i < argc; ++i) {
		ui.open_window(std::unique_ptr<Controller>(new Editor(argv[i])));
	}
	do {
		update_panels();
		doupdate();
	} while (ui.process(getch()));
	return 0;
}
