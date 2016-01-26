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

# function (objs suffixes,srcdir,objdir)
#  For all files in srcdir having one of the listed suffixes, list the path
#  its corresponding .o file would have under objdir.
objs = $(call objsubst,$(2),$(3),$(call findtype,$(1),$(2)))

