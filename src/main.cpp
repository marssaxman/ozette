// ozette
// Copyright (C) 2014-2016 Mars J. Saxman
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

#include "app/ozette.h"
#include <signal.h>
#include <stdlib.h>

static std::unique_ptr<Ozette> s_app;

static void handle_sigint(int) {
	s_app.reset();
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	(void)signal(SIGINT, handle_sigint);
	(void)signal(SIGPIPE, SIG_IGN);
	s_app.reset(new Ozette);
	for (int i = 1; i < argc; ++i) {
		s_app->edit_file(argv[i]);
	}
	s_app->run();
	return 0;
}

