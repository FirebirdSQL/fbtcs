#!/bin/sh
# The contents of this file are subject to the InterBase Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License.
#
# You may obtain a copy of the License at http://www.Inprise.com/IPL.html.
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.  The Original Code was created by Inprise
# Corporation and its predecessors.
#
# Portions created by Inprise Corporation are Copyright (C) Inprise
# Corporation. All Rights Reserved.
#
# Contributor(s): ______________________________________.

if [ "$3" = "" ]; then
	if [ "$FIREBIRD" = "" ]; then
		echo "FIREBIRD not defined."
		echo "If environment variable is not set "
		echo "the third argument for build_mingw.sh if the value of FIREBIRD"
		exit
	fi
else
	FIREBIRD="$3"
	export FIREBIRD
fi

echo "FIREBIRD=$FIREBIRD"

cd ..
FBTCS="$(pwd -W)"
export FBTCS
echo "FBTCS=$FBTCS"
cd bin

date

PATH=./bin:$FIREBIRD/bin:$PATH
export PATH
LD_LIBRARY_PATH=/usr/lib:./bin
export LD_LIBRARY_PATH
SHLIB_PATH=/usr/lib:./bin
export SHLIB_PATH
LD_RUN_PATH=/usr/lib:$FBTCS:./bin
export LD_RUN_PATH

$FIREBIRD/bin/gsec -delete qa_user1
$FIREBIRD/bin/gsec -delete qa_user2
$FIREBIRD/bin/gsec -delete qa_user3
$FIREBIRD/bin/gsec -delete qa_user4
$FIREBIRD/bin/gsec -delete qa_user5

$FIREBIRD/bin/gsec -add qa_user1 -pw qa_user1
$FIREBIRD/bin/gsec -add qa_user2 -pw qa_user2
$FIREBIRD/bin/gsec -add qa_user3 -pw qa_user3
$FIREBIRD/bin/gsec -add qa_user4 -pw qa_user4
$FIREBIRD/bin/gsec -add qa_user5 -pw qa_user5

# test v4_api15
$FIREBIRD/bin/gsec -delete guest
$FIREBIRD/bin/gsec -add guest    -pw guest
# series gf_shutdown &  gf_shut_l1
$FIREBIRD/bin/gsec -delete shut1
$FIREBIRD/bin/gsec -add shut1    -pw shut1
# series gf_shut_l1
$FIREBIRD/bin/gsec -delete shut2
$FIREBIRD/bin/gsec -add shut2    -pw shut2
# series nist3
# series procs_qa_bugs, test bug_6015
$FIREBIRD/bin/gsec -delete qatest
$FIREBIRD/bin/gsec -add qatest             -pw qatest


if [ $? != 0 ]; then
    echo "ERROR: Failure adding users"
    exit 1		# failure adding users
fi

echo test type : $1
echo test name : $2
mkdir -p ../temp
mkdir -p ../output

tcs $1 $2