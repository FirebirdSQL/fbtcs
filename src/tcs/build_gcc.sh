#!/bin/sh

# Copyright whatever... 

# This script is a real simple hack to get it going, it probably
# won't work for you, but it's simple enough that you should be
# able to follow the commands along manually

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


echo "beginning $(uname)"


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
# Get system type
#
case `uname -s` in
	SINIX-Z) 
		exeext=
		ext=sinixz
		CFLAGS="-ggdb -Wall -Wno-parentheses -DSINIXZ"
		LIBS="-lsocket -lnsl"
		CLIENTLIB="$FIREBIRD/lib/libfbembed.so"
		;;
	Linux)	
		exeext=
		ext=linux
		CFLAGS="-ggdb -Wall -Wno-parentheses -DLINUX"
		LIBS="-lstdc++ -lpthread"
		CLIENTLIB="$FIREBIRD/lib/libfbclient.so"
		;;
	FreeBSD)	
		exeext=
		ext=freebsd
		CFLAGS="-ggdb -Wall -Wno-parentheses -DFREEBSD -pthread"
		LIBS="-lstdc++"
		CLIENTLIB="$FIREBIRD/lib/libfbclient.so"
		;;
	Darwin)	
		exeext=
		ext=darwin
		CFLAGS="-ggdb -Wall -Wno-parentheses -DDARWIN"
		LIBS=
		CLIENTLIB="-framework Firebird"
		;;
	MINGW32*)
		exeext=.exe
		ext=mingw
		CFLAGS="-ggdb -mno-cygwin -Wall -Wno-parentheses -DMINGW -DWIN_NT"
		LIBS="-lmsvcrt -lws2_32"
		CLIENTLIB="$FIREBIRD/lib/fbclient_ms.lib"
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
g++ $CFLAGS $INCLUDEDIR tcs.cpp exec.cpp trns.cpp do_diffs.cpp $LIBS $CLIENTLIB -o tcs$exeext
echo "Building drop_gdb"
g++ $CFLAGS $INCLUDEDIR drop_gdb.cpp $LIBS $CLIENTLIB -o drop_gdb$exeext
echo "Building create_file"
g++ $CFLAGS $INCLUDEDIR create_file.cpp $LIBS -o create_file$exeext
echo "Building copy_file"
g++ $CFLAGS $INCLUDEDIR copy_file.cpp $LIBS -o copy_file$exeext
echo "Building compare_file"
g++ $CFLAGS $INCLUDEDIR compare_file.cpp $LIBS -o compare_file$exeext
echo "Building delete_file"
g++ $CFLAGS $INCLUDEDIR delete_file.cpp $LIBS -o delete_file$exeext
echo "Building sleep_sec"
g++ $CFLAGS $INCLUDEDIR sleep_sec.cpp $LIBS -o sleep_sec$exeext

#(cd tcs/mu; make -f makefile.$ext)

#
# create bin dir and move executables to it
#
echo "Copying files to ../bin"

destDir=../../bin

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
