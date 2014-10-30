#include "ui.h"
#include "console.h"
#include "browser.h"

int main()
{
	UI ui;
        // Build our two starting windows.
        std::unique_ptr<Window::Controller> console(new Console);
        ui.open_window(std::move(console));
        std::unique_ptr<Window::Controller> browser(new Browser);
        ui.open_window(std::move(browser));
	do {
		update_panels();
		doupdate();
	} while (ui.process(getch()));
	return 0;
}
