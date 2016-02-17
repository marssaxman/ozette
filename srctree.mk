# Copyright (C) 2016 Mars Saxman. All rights reserved.
# Permission is granted to use at your own risk and distribute this software
# in source and binary forms provided all source code distributions retain
# this paragraph and the above copyright notice. THIS SOFTWARE IS PROVIDED "AS
# IS" WITH NO EXPRESS OR IMPLIED WARRANTY.


# This is a package of functions which help automate the process of building a
# C or C++ program and reduce the need for manual editing of Makefiles, by
# finding all the files in your source tree and generating a corresponding list
# of ".o" file dependencies for your output binary. This lets you add, move,
# rename, and delete source files at will, without touching your Makefile; make
# will simply use these functions to figure it out and do the right thing.

# Imagine that your project directory looks like this:
# program/
#   Makefile
#   LICENSE
#   README
#   src/
#    main.cpp
#    sub1/
#      foo.cpp
#      bar.cpp
#    sub2/
#      baz.cpp

# Those .cpp files presumably need to be compiled into .o files which can be
# linked together into some executable, so you need a build rule like this:
# program: obj/main.o obj/sub1/foo.o obj/sub1/bar.o obj/sub2/baz.o

# You can use this library to generate such a list automatically, like this:
# include srctree.mk
# program: $(cxx_objs, src, obj)

# If you use other extensions for your source files, just call the "listobjs"
# function instead and specify anything you like:
# program: $(listobjs, "c cc cx", src, obj)



# function (map function,list):
#  Apply the function to each item in the list.
map = $(foreach item,$(2),$(call $(1),$(item)))

# function (list dir):
#  List the directory and its contents, recursively.
list = $(strip $(1) $(call map,list,$(wildcard ${1:/=}/*)))

# function (find pattern,dir):
#  Find all of the items within this directory that match the pattern.
find = $(filter $(1),$(call list,$(2)))

# function (findtype suffixes,dir):
#  Find all of the files in this directory with one of these type extensions.
findtype = $(call find,$(addprefix %.,$(1)),$(2))

# function (setprefix old,new,names)
#  Filter items matching old prefix, then substitute new prefix.
setprefix = $(patsubst $(strip $(1))%,$(strip $(2))%, $(filter $(1)%, $(3)))

# function (setsuffix suffix,names):
#  Drop the old suffix and add a new one.
setsuffix = $(addsuffix $(1),$(basename $(2)))

# function (objsubst srcdir,objdir,srcfiles)
#  List the corresponding object files for each source file in the list,
#  mapping the source file hierarchy to an equivalent object file hierarchy.
objsubst = $(call setsuffix,.o, $(call setprefix,$(1),$(2),$(3)))

# function (listobjs suffixes,srcdir,objdir)
#  For all files in srcdir having one of the listed suffixes, list the path
#  its corresponding .o file would have under objdir.
listobjs = $(call objsubst,$(2),$(3),$(call findtype,$(1),$(2)))

# function (cxx_objs srcdir,objdir)
#  List all the .o files for all the files in the source directory having
#  one of the usual C or C++ source file extensions.
cxx_objs = $(call listobjs,c cpp,$(1),$(2))

