#!/bin/bash

# Copyright whatever... 

# unix version of build script, based on build_gcc.sh, designed for native
# compilers like Sun Studio

# this script keys off value of CXX to determine the compiler

# There is also more detailed documentation in tcs/doc
# and you might want to check out interbase/firebird/fsg/TCS for
# some additonal scripts (that is the one in the interbase tree 
# not the TCS tree) 
# MOD 27-July-2001

# First I had to build the tcs programs tcs diff drop_gds
# (there is a readme there for other platforms)

# I also hand edited a script file tcs/scripts/runtcs.fb2 which does a
# cd before connecting to isc4.gdb (it only does this because I ain't 
# allowing it to run via inetd or superserver currently)


echo "beginning" $(uname)


#
# Verify and/or set FIREBIRD env variable
#
if [ "$1" = "" ]; then
	if [ "$FIREBIRD" = "" ]; then
		echo "FIREBIRD not defined."
		echo "If environment variable is not set "
		echo "the first argument for `basename $0` is the value of FIREBIRD"
		exit
	fi
else
	FIREBIRD="$1"
	export FIREBIRD
fi
echo "FIREBIRD=$FIREBIRD"

#
# Verify CXX env variable
#
if [ -z "$CXX" ]; then
	echo "CXX not defined"
	exit
fi
echo "Using compiler: $CXX"


#
# Get system type
#
case `uname -s` in
	SunOS*)	
		exeext=
		ext=solaris_SunStudio
		CFLAGS="-errtags=yes -g -xmemalign -DSOLARIS -DBSD_COMP -D__EXTENSIONS__ -D_POSIX_THREAD_SEMANTICS -D_POSIX_THREAD_PRIO_INHERIT -D_POSIX_C_SOURCE=199506L -KPIC -mt -m64 "$CFLAGS
		LIBS="-library=no%Cstd,Crun -mt -m64 -lrt -lsocket -lnsl -lresolv -lm -lpthread -lcurses"
		CLIENTLIB="$FIREBIRD/lib/libfbembed.so"
		;;

#  *)       exit 1;;
esac

#
# Show variables
#

echo "System=$ext"
echo "executable ext=$exeext"
echo "CFLAGS=$CFLAGS"
echo "LIBS=$LIBS"
echo "CLIENTLIB=$CLIENTLIB"

#
# Set FBTCS variable
#
cd ../..
if [ "$ext" = "mingw" ]; then
	FBTCS="$(pwd -W)"
else
	FBTCS="$(pwd)"
fi
export FBTCS
echo "FBTCS=$FBTCS"
cd src/tcs

#
# Make
# 
INCLUDEDIR="-I $FIREBIRD/include/"
echo "INCLUDEDIR=$INCLUDEDIR"

echo
echo
echo "Building tcs"
cmd="$CXX $CFLAGS $INCLUDEDIR tcs.cpp exec.cpp trns.cpp do_diffs.cpp $LIBS $CLIENTLIB -o tcs$exeext"
echo $cmd
$cmd || exit $?

echo "Building drop_gdb"
cmd="$CXX $CFLAGS $INCLUDEDIR drop_gdb.cpp $LIBS $CLIENTLIB -o drop_gdb$exeext"
echo $cmd
$cmd || exit $?

echo "Building create_file"
cmd="$CXX $CFLAGS $INCLUDEDIR create_file.cpp $LIBS -o create_file$exeext"
echo $cmd
$cmd || exit $?

echo "Building copy_file"
cmd="$CXX $CFLAGS $INCLUDEDIR copy_file.cpp $LIBS -o copy_file$exeext"
echo $cmd
$cmd || exit $?

echo "Building compare_file"
cmd="$CXX $CFLAGS $INCLUDEDIR compare_file.cpp $LIBS -o compare_file$exeext"
echo $cmd
$cmd || exit $?

echo "Building delete_file"
cmd="$CXX $CFLAGS $INCLUDEDIR delete_file.cpp $LIBS -o delete_file$exeext"
echo $cmd
$cmd || exit $?

echo "Building sleep_sec"
cmd="$CXX $CFLAGS $INCLUDEDIR sleep_sec.cpp $LIBS -o sleep_sec$exeext"
echo $cmd
$cmd || exit $?

#(cd tcs/mu; make -f makefile.$ext)

#
# create bin dir and move executables to it
#

destDir=../../bin
echo "Copying files to "$destDir
mkdir -p $destDir 2>nul
mv compare_file$exeext	$destDir
mv copy_file$exeext		$destDir
mv create_file$exeext	$destDir
mv delete_file$exeext	$destDir
mv drop_gdb$exeext		$destDir
mv sleep_sec$exeext		$destDir
mv tcs$exeext			$destDir

cp ../runtcs.$ext.sh $destDir/runtcs.sh
cp ../tcs.config.$ext $destDir/tcs.config
rm -f *.o

# cp tcs/mu/mu $destDir
# cp tcs/mu/libmu.a $destDir
# cp tcs/mu/client_lib.o $destDir
