#!/usr/bin/env bash
#
# Build process for C or C++ programs compiled with GCC.
#
# Find all the source files, check for changes, resolve dependencies, and
# recompile anything that needs an update. If we succeeded, link it together.
#
# The script may be configured by setting environment variables as follows.
# Name of executable to build:
: ${OUTFILE=executable}
# Directory for .o and .d files:
: ${OBJDIR=./obj}
# Source tree location:
: ${SRCDIR=./src}
# Compiler to invoke:
: ${CPP=gcc}
# Extra flags to pass the compiler on each source file:
: ${CCFLAGS=}
# Extra flags to pass the compiler when linking:
: ${LDFLAGS=}
# Standard to use when compiling .c files:
: ${STDC=c99}
# Standard to use when compiling .cpp files:
: ${STDCPP=c++11}

echo "compiling $OUTFILE"

# If there is no obj directory, we should create one.
if [[ -e $OBJDIR ]]
then
	# nothing to do
	echo -n ""
else
	echo "creating '$OBJDIR' directory..."
	mkdir $OBJDIR
fi

# Process dependencies: delete any obj files that are out of date. Each
# dependency file lists one obj file and a string of file paths it depends on.
# If any of those dependencies are newer than the obj file, we'll delete it,
# which will cause it to be recompiled on the next loop.
echo "checking dependencies..."
for dep in $(find $OBJDIR -type f -name "*.d"); do
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

# Run the main compile loop.
# Examine each source file. If there is not already a .o file for it in the
# objdir, then we must recompile it. This will happen the first time we
# compile, every time we change the source file, or every time we change one of
# the source file's dependencies.
errorfound=0
for src in $(find $SRCDIR -type f -name "*.cpp" -o -name "*.c"); do
	# make up an obj file path in our obj subdirectory
	# Strip off any leading "./" which might happen to be present
	src="${src#\./}"
	relpath="${src%/*.*}"
	mkdir -p "$OBJDIR/$relpath"
	objname="${src%.*}.o"
	obj="$OBJDIR/$objname"
	depname="${src%.*}.d"
	dep="$OBJDIR/$depname"

	# Do we need to recompile the source file?
	if [[ -e $obj && -e $dep ]] ; then
		# an object file exists, so no need to recompile
		# echo "using $objname"
		true
	else
		# no object file, so we'll compile one
		echo "compiling $src..."

		# only specify C99 if we are not compiling a c++ file
		if [[ ${src##*.} != "cpp" ]]; then
			std=$STDC
		else
			std=$STDCPP
		fi

		if $CPP $CCFLAGS -MD -std=$std -I$SRCDIR -c $src -o $obj
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
	if $CPP -o $OUTFILE $(find $OBJDIR -name "*.o") $LDFLAGS; then
		echo "done."
	else
		errorfound=1
	fi
fi

if (( !errorfound )) ; then
	ls -l $OUTFILE
fi

exit $errorfound
