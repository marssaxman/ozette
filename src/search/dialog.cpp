// ozette
// Copyright (C) 2015-2016 Mars J. Saxman
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

#include "search/dialog.h"
#include "dialog/form.h"
#include "app/path.h"

void Search::Dialog::show(UI::Frame &ctx, spec job) {
	::Dialog::Form dialog;
	dialog.fields = {
		{"Search for", job.needle},
		{"Filenames", job.filter.empty()? "*": job.filter},
		{"Directory", job.haystack, &Path::complete_dir}
	};
	dialog.commit = [](UI::Frame &ctx, ::Dialog::Form::Result &result) {
		spec job = {
			result.fields["Search for"],
			result.fields["Directory"],
			result.fields["Filenames"]
		};
		ctx.app().search_for(job);
	};
	dialog.show(ctx);
}

