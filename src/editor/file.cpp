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

#include "editor/file.h"
#include "app/path.h"
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#if defined(__linux__) || defined(__APPLE__)
#include <sys/xattr.h>
#endif

namespace {
class Descriptor {
public:
	explicit Descriptor(int fd): fd(fd) {}
	~Descriptor() { if (fd >= 0) ::close(fd); }
	Descriptor(const Descriptor &) = delete;
	Descriptor &operator=(const Descriptor &) = delete;
	int fd;
};

std::runtime_error io_error(std::string action, std::string path) {
	int code = errno;
	return std::runtime_error(action + " " + path + ": " + std::strerror(code));
}

bool same_file(const struct stat &a, const struct stat &b) {
#ifdef __APPLE__
	const auto &am = a.st_mtimespec, &bm = b.st_mtimespec;
	const auto &ac = a.st_ctimespec, &bc = b.st_ctimespec;
#else
	const auto &am = a.st_mtim, &bm = b.st_mtim;
	const auto &ac = a.st_ctim, &bc = b.st_ctim;
#endif
	return a.st_dev == b.st_dev && a.st_ino == b.st_ino &&
		a.st_size == b.st_size && a.st_mode == b.st_mode &&
		a.st_uid == b.st_uid && a.st_gid == b.st_gid && a.st_nlink == b.st_nlink &&
		am.tv_sec == bm.tv_sec && am.tv_nsec == bm.tv_nsec &&
		ac.tv_sec == bc.tv_sec && ac.tv_nsec == bc.tv_nsec;
}

bool inspect(std::string path, struct stat &info) {
	if (stat(path.c_str(), &info) == 0) return true;
	if (errno == ENOENT) return false;
	throw io_error("Can't inspect", path);
}

std::string resolve(std::string path) {
	char *resolved = realpath(path.c_str(), nullptr);
	if (!resolved) throw io_error("Can't resolve", path);
	std::string out(resolved);
	free(resolved);
	return out;
}

std::string destination(std::string path) {
	struct stat info;
	if (lstat(path.c_str(), &info) == 0) return resolve(path);
	if (errno != ENOENT) throw io_error("Can't write", path);
	size_t slash = path.find_last_of('/');
	return resolve(path.substr(0, slash + 1)) + "/" + path.substr(slash + 1);
}

struct Temporary {
	explicit Temporary(std::string directory):
			name(directory + "/.ozette-XXXXXX"), file(mkstemp(&name[0])) {
		if (file.fd < 0) throw io_error("Can't create temporary file in", directory);
	}
	~Temporary() { if (!name.empty()) unlink(name.c_str()); }
	std::string name;
	Descriptor file;
};

void copy_attributes(int from, int to, std::string path) {
#if defined(__linux__) || defined(__APPLE__)
#ifdef __APPLE__
	auto list = [from](char *buf, size_t size) { return flistxattr(from, buf, size, 0); };
#else
	auto list = [from](char *buf, size_t size) { return flistxattr(from, buf, size); };
#endif
	ssize_t size = list(nullptr, 0);
	if (size < 0 && (errno == ENOTSUP || errno == EOPNOTSUPP)) return;
	if (size < 0) throw io_error("Can't read attributes of", path);
	if (size == 0) return;
	std::vector<char> names(size);
	size = list(names.data(), names.size());
	if (size < 0) throw io_error("Can't read attributes of", path);
	for (ssize_t offset = 0; offset < size;) {
		const char *name = names.data() + offset;
#ifdef __APPLE__
		auto get = [from, name](void *buf, size_t size) {
			return fgetxattr(from, name, buf, size, 0, 0);
		};
#else
		auto get = [from, name](void *buf, size_t size) {
			return fgetxattr(from, name, buf, size);
		};
#endif
		ssize_t length = get(nullptr, 0);
		if (length < 0) throw io_error("Can't read attributes of", path);
		std::vector<char> value(length);
		length = get(value.data(), value.size());
		if (length < 0) throw io_error("Can't read attributes of", path);
#ifdef __APPLE__
		int result = fsetxattr(to, name, value.data(), length, 0, 0);
#else
		int result = fsetxattr(to, name, value.data(), length, 0);
#endif
		if (result < 0) throw io_error("Can't preserve attributes of", path);
		offset += std::strlen(name) + 1;
	}
#endif
}
} // namespace

std::string Editor::File::read(std::string path) {
	path = Path::absolute(path);
	Descriptor file(open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC));
	if (file.fd < 0) {
		if (errno != ENOENT) throw io_error("Can't read", path);
		struct stat link;
		if (lstat(path.c_str(), &link) == 0) {
			throw std::runtime_error("Missing link target: " + path);
		}
		if (errno != ENOENT) throw io_error("Can't read", path);
		_path = path;
		_exists = false;
		return "";
	}
	struct stat info;
	if (fstat(file.fd, &info)) throw io_error("Can't read", path);
	if (!S_ISREG(info.st_mode)) {
		throw std::runtime_error("Not a regular file: " + path);
	}
	std::string text;
	char buf[8192];
	for (;;) {
		ssize_t count = ::read(file.fd, buf, sizeof(buf));
		if (count == 0) break;
		if (count < 0) {
			if (errno == EINTR) continue;
			throw io_error("Can't read", path);
		}
		text.append(buf, count);
	}
	struct stat after;
	if (fstat(file.fd, &after)) throw io_error("Can't read", path);
	if (!same_file(info, after) || !inspect(path, after) || !same_file(info, after)) {
		throw Changed("File changed while reading: " + path);
	}
	_path = path;
	_info = info;
	_exists = true;
	return text;
}

void Editor::File::write(std::string path, const std::string &text, bool overwrite) {
	path = Path::absolute(path);
	std::string target = destination(path);
	std::string directory = target.substr(0, target.find_last_of('/'));
	if (directory.empty()) directory = "/";
	struct stat info;
	bool exists = inspect(target, info);
	if (!overwrite) {
		if (path == _path) {
			if (exists != _exists || (exists && !same_file(info, _info))) {
				throw Changed("File changed on disk: " + path);
			}
		} else if (exists) {
			throw Changed("File already exists: " + path);
		}
	}
	Descriptor original(-1);
	mode_t mode;
	if (exists) {
		if (!S_ISREG(info.st_mode)) throw std::runtime_error("Not a regular file: " + path);
		original.fd = open(target.c_str(), O_WRONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
		if (original.fd < 0) throw io_error("Can't write", path);
		struct stat opened;
		if (fstat(original.fd, &opened)) throw io_error("Can't inspect", path);
		if (!same_file(info, opened)) throw Changed("File changed on disk: " + path);
		if (info.st_nlink > 1) {
			throw std::runtime_error("File has multiple hard links; use Save As: " + path);
		}
		mode = info.st_mode & 07777;
	} else {
		mode_t mask = umask(0);
		umask(mask);
		mode = 0666 & ~mask;
	}
	Descriptor parent(open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC));
	if (parent.fd < 0) throw io_error("Can't open directory", directory);
	Temporary temp(directory);
	if (fcntl(temp.file.fd, F_SETFD, FD_CLOEXEC) < 0) throw io_error("Can't prepare", path);
	for (size_t offset = 0; offset < text.size();) {
		ssize_t count = ::write(temp.file.fd, text.data() + offset, text.size() - offset);
		if (count < 0 && errno == EINTR) continue;
		if (count == 0) errno = EIO;
		if (count <= 0) throw io_error("Can't write", path);
		offset += count;
	}
	if (exists) {
		struct stat created;
		if (fstat(temp.file.fd, &created)) throw io_error("Can't inspect", path);
		if (created.st_uid != info.st_uid || created.st_gid != info.st_gid) {
			if (fchown(temp.file.fd, info.st_uid, info.st_gid)) {
				throw io_error("Can't preserve ownership of", path);
			}
		}
		copy_attributes(original.fd, temp.file.fd, path);
	}
	if (fchmod(temp.file.fd, mode)) throw io_error("Can't preserve permissions of", path);
	if (fsync(temp.file.fd)) throw io_error("Can't sync", path);
	int fd = temp.file.fd;
	temp.file.fd = -1;
	if (::close(fd)) throw io_error("Can't close", path);
	struct stat current;
	bool present = inspect(path, current);
	if (present != exists || (present && !same_file(info, current)) ||
			destination(path) != target) {
		throw Changed("File changed while saving: " + path);
	}
	if (rename(temp.name.c_str(), target.c_str())) throw io_error("Can't replace", path);
	temp.name.clear();
	_path = path;
	_exists = true;
	if (stat(target.c_str(), &_info)) throw io_error("Can't inspect saved file", path);
	if (fsync(parent.fd)) throw io_error("File written, but can't sync directory", directory);
}
