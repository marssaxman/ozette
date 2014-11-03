#include "ui.h"
#include "console.h"
#include "browser.h"
#include "editor.h"
#include <signal.h>

static std::unique_ptr<UI> s_ui;

static void finish(int sig)
{
	s_ui.reset();
	exit(0);
}

int main(int argc, char **argv)
{
	(void)signal(SIGINT, finish);
	s_ui.reset(new UI);
        // Build our two starting windows.
        std::unique_ptr<Controller> console(new Console);
        s_ui->open_window(std::move(console));
        std::unique_ptr<Controller> browser(new Browser);
        s_ui->open_window(std::move(browser));
	// If we got a list of arguments, create editors for them.
	for (int i = 1; i < argc; ++i) {
		s_ui->open_window(std::unique_ptr<Controller>(new Editor(argv[i])));
	}
	timeout(20);
	do {
		update_panels();
		doupdate();
	} while (s_ui->process(getch()));
	return 0;
}

