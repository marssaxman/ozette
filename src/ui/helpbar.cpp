#include "helpbar.h"

HelpBar::Label::Label(char m, bool c, std::string t):
	mnemonic(m),
	is_ctrl(c),
	text(t)
{
}

HelpBar::Label::Label():
	mnemonic(0),
	is_ctrl(false)
{
}

