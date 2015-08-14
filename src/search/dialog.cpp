//
// ozette
// Copyright (C) 2015 Mars J. Saxman
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

#include "search/dialog.h"
#include "ui/form.h"
#include "app/path.h"

void Search::Dialog::show(UI::Frame &ctx, spec job)
{
	UI::Form dialog;
	dialog.fields = {
		{"Find", job.needle},
		{"Files", job.filter},
		{"Directory", job.haystack, &Path::complete_dir}
	};
	dialog.commit = [](UI::Frame &ctx, UI::Form::Result &result)
	{
		spec job = {
			result.fields["Find"],
			result.fields["Directory"],
			result.fields["Files"]
		};
		ctx.app().search(job);
	};
	dialog.show(ctx);
}

