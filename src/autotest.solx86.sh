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

export FIREBIRD=/usr/local/firebird

export ISC_USER=sysdba

export ISC_PASSWORD=masterkey

echo "FIREBIRD=$FIREBIRD"
echo "ISC_USER=$ISC_USER"
echo "ISC_PASSWORD=$ISC_PASSWORD"

cd ../bin

date
./runtcs.sh rs CF_ISQL
./runtcs.sh rs C_SQL_PRED
./runtcs.sh rs ISC_ARRBLOB_CK
./runtcs.sh rs DSQL_DOMAINS
./runtcs.sh rs DSQL_EXCEPTIONS
./runtcs.sh rs SQL_EVENTS
./runtcs.sh rs C_SQL_BLOB
./runtcs.sh rs C_SQL_JOIN
./runtcs.sh rs  V4_AUTO_COMMIT
#V4_EXCLUSIVE	9
./runtcs.sh rs IDML_C_CHARSET
./runtcs.sh rs IDML_C_SUBQUERYS
./runtcs.sh rs EVENT_MISC
./runtcs.sh rs EXT_LEV_0
./runtcs.sh rs GF_SHUTDOWN
./runtcs.sh rs QA_PROC_IES_ERR
./runtcs.sh rs REF_INT_INDX
./runtcs.sh rs RICH_FLD_GEN_V40
./runtcs.sh rs RI_TESTS
./runtcs.sh rs V4_AUTO_DDL_DSQL
./runtcs.sh rs V4_TRANS_BLOB
./runtcs.sh rs QA_PROCS_ISQL
./runtcs.sh rs FB_SQL
./runtcs.sh rs DDL
./runtcs.sh rs SVC_API
./runtcs.sh rs GSTAT
date

