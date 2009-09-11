#!/usr/local/bin/bash
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

if [ "$1" = "" ]; then
	echo "Test type not defined."
	echo "allowed values are rt (run test), rs (run series) and rms(run meta series)"
	echo "the first argument for `basename $0` is the test type"
	exit
fi

if [ "$2" = "" ]; then
	echo "Test/series/metaseries name not defined."
	echo "the second argument for `basename $0` is the Test/series/metaseries name"
	exit
fi

if [ "$FIREBIRD" = "" ]; then
	echo "FIREBIRD environment variable not defined."
	echo "You need to define it to run the tests"
	exit
fi

if [ "$ISC_USER" = "" ]; then
	echo "ISC_USER environment variable not defined."
	echo "You need to define it to run the tests"
	exit
fi

if [ "$ISC_PASSWORD" = "" ]; then
	echo "ISC_PASSWORD environment variable not defined."
	echo "You need to define it to run the tests"
	exit
fi

echo "FIREBIRD=$FIREBIRD"
echo "ISC_USER=$ISC_USER"
echo "ISC_PASSWORD=$ISC_PASSWORD"

cd ..
FBTCS="$(pwd)"
export FBTCS
echo "FBTCS=$FBTCS"
cd bin

date

PATH=./bin:$FIREBIRD/bin:$PATH
export PATH
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib:./bin:$FIREBIRD/lib:$FIREBIRD/intl
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
rm -f ../temp/*

./tcs $1 $2
