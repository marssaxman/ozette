#include "lindi.h"
#include <signal.h>
#include <sys/wait.h>

static std::unique_ptr<Lindi> s_app;

static void handle_sigint(int)
{
	s_app.reset();
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
	std::list<std::string> args;
	for (int i = 1; i < argc; ++i) {
		args.emplace_back(argv[i]);
	}
	s_app.reset(new Lindi(args));
	s_app->run();
	return 0;
}

