#ifndef UI_HELPBAR_H
#define UI_HELPBAR_H

#include <string>

namespace HelpBar {
struct Label {
	Label(char m, bool c, std::string t);
	Label();
	char mnemonic;
	bool is_ctrl;
	std::string text;
};
struct Panel {
        static const size_t kWidth = 6;
        static const size_t kHeight = 2;
        Label label[kHeight][kWidth];
};
} // namespace HelpBar

#endif //UI_HELPBAR_H
