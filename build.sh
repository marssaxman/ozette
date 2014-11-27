#!/usr/bin/env bash
#
# Project build script.
# Find all the source files involved in our project and compile them.
# If we compiled successfuly, link an executable.
#
# The first argument should be the name of the executable to build.
# If there are more arguments, they will be taken as the names of root
# directories for your source tree.  We will search them recursively for
# source & header files. If you do not specify any source directories, we will
# assume the current working directory. We expect that the current directory
# is your "project directory" and will create a subdirectory there for object
# and dependency files.
#
# example:   build.sh foo ./objs/ ./foo/src/
#
# arg immediately after the target name is the dir to put object files in


# standard compiler options; adjust these as you please
ccflags="-Wall -Wno-endif-labels -g -falign-functions=4"
ccflags="$ccflags -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS"
ccflags="$ccflags -std=c++11"
# compiler command - needs to be gcc so we can have the -MD option
# without -MD, we have no way to generate dependency information.
cpp="gcc"
ldflags="-lpanel -lncurses -lpthread -lstdc++"

# Project structure:
#
# We will search in all of the rootdirs for source & header files.
# Intermediate .o files will be stored in the objdir.
# If we compile everything successfully, we will link up the outfile.
outfile=$1
echo "compiling $1"
shift

objdir=$1
shift

if [ "$1" != "" ]; then
	rootdirs="$*"
else
	rootdirs="."
fi


# Lindi-specific variant of the standard martian build process:
# generate a C source file representation of the help file.
if [ ./HELP -nt src/app/help.cpp ] ; then
	xxd -i HELP src/app/help.cpp
fi

# Begin!

# If there is no obj directory, we should create one.
if [[ -e $objdir ]]
then
	# nothing to do
	echo -n ""
else
	# I'd like to call the objdir "obj", as is traditional, but make has an
	# interesting habit of noticing that an "obj" dir exists and cd'ing into
	# it, automatically, before invoking the build rules. This creates certain
	# difficulties, and it's not worth bothering with.
	echo "creating '$objdir' directory..."
	mkdir $objdir
fi

# Process dependencies: delete any obj files that are out of date. Each
# dependency file lists one obj file and a string of file paths it depends on.
# If any of those dependencies are newer than the obj file, we'll delete it,
# which will cause it to be recompiled on the next loop.
echo "checking dependencies..."
for dep in $(find $objdir -type f -name "*.d"); do
	depfiledata=$(cat $dep)
	objtarget=${depfiledata%%:*}
	if [[ -e $objtarget ]]
	then
		deplist=${depfiledata##*:}
		for checkfile in $deplist; do
			# Ignore the backslash line-continuation entries gcc puts in the
			# dep file
			if [ "$checkfile" = "\\" ] ; then
				continue
			fi
			# If the obj file depends on a file which is newer, delete it.
			# If the obj file depends on a file which does not exist, that is
			# also a good reason to delete it.
			if [ $checkfile -nt $objtarget -o ! -e $checkfile ] ; then
				rm $objtarget
				break
			fi
		done
	fi
done

# List all of the directories in our project, then generate include directives
# for each one. We need to tell the compiler where to find header files. We
# might as well do this ahead of time so we can reuse the include arguments
# for each compilation.
include=""
dirs=$(find $rootdirs -type d | grep -v -e "/\.")
for i in $dirs; do
	include="$include -I$i"
done

# Run the main compile loop.
# Examine each source file. If there is not already a .o file for it in the
# objdir, then we must recompile it. This will happen the first time we
# compile, every time we change the source file, or every time we change one of
# the source file's dependencies.
errorfound=0
for src in $(find $rootdirs -type f -name "*.cpp" -o -name "*.c"); do
	# get just the file name, no path
	srcname="${src##*/}"
	# make up an obj file path in our obj subdirectory
	objname="${srcname%.*}.o"
	obj="$objdir/$objname"
	depname="${srcname%.*}.d"
	dep="$objdir/$depname"

	# Do we need to recompile the source file?
	if [[ -e $obj && -e $dep ]] ; then
		# an object file exists, so no need to recompile
		# echo "using $objname"
		true
	else
		# no object file, so we'll compile one
		echo "compiling $srcname..."

		# only specify C99 if we are not compiling a c++ file
		if [[ ${src##*.} != "cpp" ]]; then
			cstd="-std=c99"
		else
			cstd=""
		fi

		if $cpp $ccflags -MD $cstd $include -c $src -o $obj
		then
			echo -n ""
		else
			errorfound=1
		fi
	fi
done

# If we successfully compiled all of the source files, link the executable.
if (( !errorfound )) ; then
	echo "linking..."
	# use gcc to link an executable
	if $cpp -o $outfile $(find $objdir -name "*.o") $ldflags; then
		echo "done."
	else
		errorfound=1
	fi
fi

if (( !errorfound )) ; then
	ls -l $outfile
fi

exit $errorfound
