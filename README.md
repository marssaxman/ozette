Ozette is an editing shell for software development. It combines a directory
browser, a text file editor, and a command console within a tabbed window
interface modelled on pico/nano, using a control-key scheme based on familiar
desktop application conventions.

Ozette is an opinionated tool designed around the way I like to write code, and
universal usefulness is not a goal. I am releasing it in case there are other
people with a similar development style who might happen to like it.

Ozette is not intended to compete with complex, sophisticated editors like vi or
emacs, because I spend a lot more time thinking about code than writing it and
have never felt like complex editors were worth the bother. Its editor is very
simple but it offers everything I actually use.

My focus for future development will be on more integrated, IDE-like features:
integration with git/svn, a better console capable of running gdb/lldb, and
syntax awareness for editor highlighting and identifier search.

====

Build it:

	make

Install it in /usr/bin/:

	sudo make install

Open the browser in the current directory:

	ozette

Edit a specific file or files, with no browser:

	ozette foo bar baz




