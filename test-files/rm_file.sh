machine=`uname -s`
node=`uname -n`
echo $WHERE_GDB_EXTERNAL > awk.tst

echo $WHERE_GDB > ls.out
proto=`cut -c 1,2 ls.out`

if [ $machine = "Windows_NT" ] || [ $machine = "Windows_95" ] || [ $machine = "Windows_98" ]
	then
		if [ $WHERE_GDB = $WHERE_GDB_EXTERNAL ]
			then
				rm -f WHERE_GDB_EXTERNAL:external.dat 
			else
				if [ $proto = "//" ]
					then
						machine_name=`awk -F"/" '{print $3}' ls.out`
					else
						machine_name=`awk -F":" '{print $1}' ls.out` 
				fi
			echo `awk -F":" '{print $1}' awk.tst` > awk.out
			server_testbed=`cat awk.out`
			ls_string=//$machine_name/$server_testbed/testbed/external.dat
			rm -f $ls_string 
		fi
	else
		if [ $WHERE_GDB = $WHERE_GDB_EXTERNAL ]
			then
				rm -f WHERE_GDB_EXTERNAL:external.dat 
			else
				if [ $proto = "//" ]
					then
						machine_name=`awk -F"/" '{print $3}' ls.out`
					else
						machine_name=`awk -F":" '{print $1}' ls.out` 
				fi
				if [ $machine_name = "localhost" ] || [ $machine_name = "127.0.0.1" ]
					then
						rm -f WHERE_GDB_EXTERNAL:external.dat
					else
						rsh $machine_name /bin/rm -f WHERE_GDB_EXTERNAL:external.dat
				fi
		fi
fi
