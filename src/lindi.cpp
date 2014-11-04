#include "lindi.h"
#include "browser.h"
#include "editor.h"

Lindi::Lindi(std::list<std::string> args)
{
        std::unique_ptr<Controller> browser(new Browser);
        _ui.open_window(std::move(browser));
	for (auto &arg: args) {
		std::unique_ptr<Controller> ed(new Editor(arg));
		_ui.open_window(std::move(ed));
	}
}

void Lindi::run()
{
	timeout(20);
	do {
		update_panels();
		doupdate();
	} while (_ui.process(getch()));
}

