#include "console.h"
#include "popenRWE.h"
#include <fcntl.h>

// what to do about SIGCHLD?

Console::Console(std::string command)
{
	int pipe[3];
	_pid = popenRWE(pipe, command.c_str());
	if (_pid < 0) {
		exit(_pid);
	}
	_stdin = pipe[0];
	_stdout = pipe[1];
	_stderr = pipe[2];
	set_nonblocking(_stdin);
	set_nonblocking(_stdout);
	set_nonblocking(_stderr);
}

void Console::set_nonblocking(int fd)
{
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

Console::~Console()
{
	int pipe[3] = {_stdin, _stdout, _stderr};
	pcloseRWE(_pid, pipe);
}

void Console::paint(WINDOW *view)
{
}

bool Console::process(WINDOW *view, int ch, App &app)
{
	write(_stdin, &ch, sizeof(char));
	return true;
}

bool Console::poll(WINDOW *view, App &app)
{
	// Try to read some data from the shell process.
	// If data came in, write it to the window.
	const size_t BUFSIZE = 1024;
	char buf[BUFSIZE];
        ssize_t nread = 0;
	while (nread = read(_stdout, buf, BUFSIZE), nread > 0) {
		receive(view, buf, (size_t)nread);
	}
	return true;
}

void Console::receive(WINDOW *view, char *buf, size_t bytes)
{
	wmove(view, _cursy, _cursx);
	waddnstr(view, buf, bytes);
	getyx(view, _cursy, _cursx);
}
