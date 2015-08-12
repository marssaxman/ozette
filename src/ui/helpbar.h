//
// ozette
// Copyright (C) 2014-2015 Mars J. Saxman
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#ifndef UI_HELPBAR_H
#define UI_HELPBAR_H

#include <string>
#include <initializer_list>

namespace UI {
namespace HelpBar {
struct Label {
	Label(std::initializer_list<std::string> items);
	Label() {}
	char mnemonic[2] = {0,0};
	std::string text;
};
struct Panel {
    static const size_t kWidth = 6;
    static const size_t kHeight = 2;
    Label label[kHeight][kWidth];

	void help()      { label[1][5] = {"^?", "Help"}; }
	void quit()      { label[1][0] = {"^Q", "Quit"}; }
	void close()     { label[1][0] = {"^W", "Close"}; }
	void escape()    { label[1][0] = {"^[", "Escape"}; }
	void open()      { label[0][0] = {"^O", "Open"}; }
	void new_file()  { label[0][1] = {"^N", "New File"}; }
	void cut()       { label[0][0] = {"^X", "Cut"}; }
	void copy()      { label[0][1] = {"^C", "Copy"}; }
	void paste()     { label[0][2] = {"^V", "Paste"}; }
	void to_line()   { label[0][4] = {"^L", "To Line"}; }
	void save()      { label[1][1] = {"^S", "Save"}; }
	void save_as()   { label[1][2] = {"^A", "Save As"}; }
	void find()      { label[0][5] = {"^F", "Find"}; }
	void search()    { label[0][5] = {"F4", "Search"}; }
	void undo()      { label[1][4] = {"^Z", "Undo"}; }
	void redo()      { label[1][3] = {"^Y", "Redo"}; }
	void execute()   { label[1][1] = {"^E", "Execute"}; }
	void directory() { label[1][2] = {"^D", "Directory"}; }
	void build()     { label[1][4] = {"F5", "Build"}; }
	void kill()      { label[0][0] = {"^K", "Kill"}; }
	void yes()       { label[0][0] = {" Y", "Yes"}; }
	void no()        { label[0][1] = {" N", "No"}; }
};
} // namespace HelpBar
} // namespace UI

#endif //UI_HELPBAR_H
