#ifndef CONSOLE_H
#define CONSOLE_H

#include "controller.h"

class Console : public Controller
{
public:
	explicit Console(std::string command);
	~Console();
	virtual void paint(WINDOW *view, bool active) override;
	virtual bool process(WINDOW *view, int ch, App &app) override;
	virtual bool poll(WINDOW *view, App &app) override;
	virtual std::string title() const override { return "Console"; }
protected:
	static void set_nonblocking(int fd);
	void receive(WINDOW *view, char *buf, size_t bytes);
private:
	int _pid = 0;
	// These are the file descriptors for the streams
	// as seen from the shell process' point of view.
	int _stdin = 0;
	int _stdout = 0;
	int _stderr = 0;
	// Location of input cursor.
	int _cursx = 0;
	int _cursy = 0;
};

#endif // CONSOLE_H
