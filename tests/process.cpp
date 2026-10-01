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

#include "doctest.h"
#include "process/subproc.h"
#include "process/popenRWE.h"
#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <memory>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <thread>

namespace {
using Clock = std::chrono::steady_clock;

struct Output {
	std::string out, err;
};

void finish(Process::Subproc &proc, Output &output) {
	auto deadline = Clock::now() + std::chrono::seconds(5);
	do {
		proc.read_out(output.out);
		proc.read_err(output.err);
		if (!proc.poll()) return;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (Clock::now() < deadline);
	FAIL("subprocess did not finish");
}

std::unique_ptr<Process::Subproc> command(const char *text) {
	const char *args[] = {"sh", "-c", text, nullptr};
	return std::unique_ptr<Process::Subproc>(new Process::Subproc(args[0], args));
}

pid_t reported_pid(Process::Subproc &proc, Output &output) {
	auto deadline = Clock::now() + std::chrono::seconds(5);
	while (output.out.find('\n') == std::string::npos && Clock::now() < deadline) {
		proc.read_out(output.out);
		proc.read_err(output.err);
		proc.poll();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	REQUIRE(output.out.find('\n') != std::string::npos);
	return std::stoi(output.out);
}
} // namespace

TEST_CASE("subprocess pipes remain nonblocking without assertions") {
	const char *args[] = {"sh", "-c", "sleep .05", nullptr};
	int pipes[3] = {-1,-1,-1};
	pid_t pid = popenRWE(pipes, args[0], args);
	REQUIRE(pid > 0);
	for (int fd: pipes) {
		CHECK((fcntl(fd, F_GETFL) & O_NONBLOCK) != 0);
		CHECK((fcntl(fd, F_GETFD) & FD_CLOEXEC) != 0);
		close(fd);
	}
	int status = 0;
	REQUIRE(waitpid(pid, &status, 0) == pid);
	CHECK(WIFEXITED(status));
}

TEST_CASE("subprocess drains both streams before reporting completion") {
	auto proc = command("i=0; while [ $i -lt 4000 ]; do printf 'output:0123456789\n'; printf 'error:0123456789\n' >&2; i=$((i+1)); done; exit 7");
	Output output;
	finish(*proc, output);
	CHECK(output.out.size() == 4000 * 18);
	CHECK(output.err.size() == 4000 * 17);
	CHECK(output.out.substr(output.out.size() - 18) == "output:0123456789\n");
	CHECK(proc->result().error == 0);
	CHECK(proc->result().exit_code == 7);
	CHECK(proc->result().message() == "exit 7");
}

TEST_CASE("subprocess receives EOF on unused stdin") {
	auto proc = command("cat; printf done");
	Output output;
	finish(*proc, output);
	CHECK(output.out == "done");
	CHECK(proc->result().exit_code == 0);
}

TEST_CASE("subprocess retains output after another stream closes") {
	auto proc = command("exec 2>&-; sleep .03; printf tail; exit 4");
	Output output;
	finish(*proc, output);
	CHECK(output.out == "tail");
	CHECK(output.err.empty());
	CHECK(proc->result().exit_code == 4);
}

TEST_CASE("subprocess observes silent exits and signals") {
	SUBCASE("closed streams do not imply process completion") {
		auto proc = command("exec 1>&- 2>&-; sleep .03; exit 3");
		Output output;
		finish(*proc, output);
		CHECK(proc->result().exit_code == 3);
	}
	SUBCASE("signal termination is recorded") {
		auto proc = command("kill -TERM $$");
		Output output;
		finish(*proc, output);
		CHECK(proc->result().signal == SIGTERM);
		CHECK(proc->result().message() == "signal " + std::to_string(SIGTERM));
	}
}

TEST_CASE("subprocess launch errors do not poll unrelated children") {
	const char *args[] = {"/no/such/ozette-command", nullptr};
	Process::Subproc proc(args[0], args);
	CHECK(proc.result().error == ENOENT);
	CHECK_FALSE(proc.poll());
	CHECK(proc.result().message().find("process error:") == 0);
}

TEST_CASE("subprocess reports descriptor exhaustion and preserves existing descriptors") {
	pid_t child = fork();
	REQUIRE(child >= 0);
	if (child == 0) {
		struct rlimit limit;
		if (getrlimit(RLIMIT_NOFILE, &limit) < 0) _exit(2);
		limit.rlim_cur = 32;
		if (setrlimit(RLIMIT_NOFILE, &limit) < 0) _exit(2);
		while (open("/dev/null", O_RDONLY) >= 0) {}
		const char *args[] = {"true", nullptr};
		Process::Subproc proc(args[0], args);
		if (proc.result().error != EMFILE || proc.poll()) _exit(3);
		if (fcntl(STDIN_FILENO, F_GETFD) < 0 || fcntl(STDOUT_FILENO, F_GETFD) < 0) _exit(4);
		_exit(0);
	}
	int status = 0;
	REQUIRE(waitpid(child, &status, 0) == child);
	CHECK(WIFEXITED(status));
	CHECK(WEXITSTATUS(status) == 0);
}

TEST_CASE("subprocess cancellation escalates without blocking") {
	auto proc = command("trap '' TERM; printf '%s\n' $$; exec sleep 30");
	Output output;
	pid_t pid = reported_pid(*proc, output);
	auto start = Clock::now();
	proc->cancel();
	CHECK(Clock::now() - start < std::chrono::milliseconds(100));
	CHECK(proc->poll());
	finish(*proc, output);
	CHECK(proc->result().cancelled);
	CHECK(proc->result().signal == SIGKILL);
	CHECK(proc->result().message() == "cancelled");
	CHECK(kill(pid, 0) < 0);
}

TEST_CASE("subprocess cancellation includes children when the leader exits") {
	std::string script = "trap 'wait $child; exit 0' TERM; sleep 30 & child=$!; printf '%s\n' $child; wait";
	auto proc = command(script.c_str());
	Output output;
	pid_t child = reported_pid(*proc, output);
	proc->cancel();
	finish(*proc, output);
	CHECK(proc->result().cancelled);
	CHECK(proc->result().exit_code == 0);
	CHECK(kill(child, 0) < 0);
}

TEST_CASE("destroying a subprocess kills and reaps it without waiting") {
	auto proc = command("trap '' TERM; printf '%s\n' $$; exec sleep 30");
	Output output;
	pid_t pid = reported_pid(*proc, output);
	auto start = Clock::now();
	proc.reset();
	CHECK(Clock::now() - start < std::chrono::milliseconds(100));
	auto deadline = Clock::now() + std::chrono::seconds(5);
	while (kill(pid, 0) == 0 && Clock::now() < deadline) {
		Process::Subproc::reap();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	CHECK(kill(pid, 0) < 0);
	CHECK(waitpid(pid, nullptr, WNOHANG) < 0);
	CHECK(errno == ECHILD);
}
