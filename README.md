Ozette is an editing shell for software development. It combines a directory
browser and a text file editor within a tabbed window interface, modelled on
the style of pico/nano, which replaces the traditional terminal-oriented control
key bindings with a scheme based on familiar desktop application conventions.

![editor screenshot](screenshots/2017-09-29_10-50-22.png?raw=true)

![browser screenshot](screenshots/2017-09-29_10-54-10.png?raw=true)

The browser offers a directory-wide regex search, and the editor has local
search and replace. F5 invokes 'make' in the current directory and captures
output in a new tab. The editor offers syntax highlighting for C, C++, Ruby,
Python, Javascript, Go, and protobufs.

====

Ozette is an opinionated tool; I wrote it for my own use, and it is tailored to
my personal preferences. It began as a quick hack, solving one problem for one
project, then grew up more quickly and smoothly than I had expected. My needs
are simple, it turns out, since I spend more time thinking about code than I do
writing it, and I've used ozette exclusively for several years now.

I have released the code largely for the interest of other dev-tool enthusiasts,
who might enjoy the opportunity for a look inside another example of the genre.

While I would be delighted if others found Ozette useful and chose to adopt it
for their own purposes, the broad availability and wide variety of other, more
mature, powerful, configurable, sophisticated, popular, and innovative editors
makes that prospect seem unlikely. I have therefore put no effort into making
Ozette configurable or scriptable, and I have done no testing beyond my regular
daily use.

If you do happen to find that this editor suits your taste, I would be happy
to hear from you, and to discuss any issues you might encounter with it.

====

Build it:

	sudo apt install libncurses5-dev
	make

Install it in /usr/bin/:

	sudo make install

Open the browser in the current directory:

	ozette

Edit a specific file or files, with no browser:

	ozette foo bar baz




