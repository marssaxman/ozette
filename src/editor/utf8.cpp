// ozette
// Copyright (C) 2026 Mars J. Saxman
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

#include "editor/utf8.h"

namespace Editor { namespace UTF8 {
Character decode(const std::string &text, size_t offset) {
	if (offset >= text.size()) return {0, 0};
	unsigned char lead = text[offset];
	if (lead < 0x80) return {lead, 1};
	const Character invalid = {0xFFFD, 1};
	size_t length;
	char32_t value, minimum;
	if (lead >= 0xC2 && lead <= 0xDF) {
		length = 2; value = lead & 0x1F; minimum = 0x80;
	} else if (lead >= 0xE0 && lead <= 0xEF) {
		length = 3; value = lead & 0x0F; minimum = 0x800;
	} else if (lead >= 0xF0 && lead <= 0xF4) {
		length = 4; value = lead & 0x07; minimum = 0x10000;
	} else return invalid;
	if (text.size() - offset < length) return invalid;
	for (size_t i = 1; i < length; ++i) {
		unsigned char byte = text[offset + i];
		if ((byte & 0xC0) != 0x80) return invalid;
		value = (value << 6) | (byte & 0x3F);
	}
	if (value < minimum || value > 0x10FFFF ||
			(value >= 0xD800 && value <= 0xDFFF)) return invalid;
	return {value, length};
}
} } // namespace Editor::UTF8

