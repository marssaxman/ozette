#include "ui.h"
#include "console.h"
#include "browser.h"
#include "editor.h"
#include <signal.h>
#include <sys/wait.h>

static std::unique_ptr<UI> s_ui;

static void handle_sigint(int)
{
	s_ui.reset();
	exit(0);
}

static void handle_sigchld(int)
{
	pid_t pid = 0;
	do {
		pid = waitpid(-1, NULL, WNOHANG);
	} while (pid > 0);
}

int main(int argc, char **argv)
{
	(void)signal(SIGINT, handle_sigint);
	(void)signal(SIGCHLD, handle_sigchld);
	s_ui.reset(new UI);
        std::unique_ptr<Controller> browser(new Browser);
        s_ui->open_window(std::move(browser));
	for (int i = 1; i < argc; ++i) {
		std::unique_ptr<Controller> ed(new Editor(argv[i]));
		s_ui->open_window(std::move(ed));
	}
	timeout(20);
	do {
		update_panels();
		doupdate();
	} while (s_ui->process(getch()));
	return 0;
}

