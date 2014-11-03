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
	// Leftmost window is the lindi project browser.
        std::unique_ptr<Controller> browser(new Browser);
        s_ui->open_window(std::move(browser));
	// Next window is a console with the login shell.
        std::unique_ptr<Controller> console(new Console("ping -c 60 8.8.4.4"));
        s_ui->open_window(std::move(console));
	// If we got a list of arguments, create editors for them.
	for (int i = 1; i < argc; ++i) {
		std::string editcmd = "nano ";
		auto ed = new Console(editcmd + argv[i]);
		s_ui->open_window(std::unique_ptr<Controller>(ed));
	}
	timeout(20);
	do {
		update_panels();
		doupdate();
	} while (s_ui->process(getch()));
	return 0;
}

