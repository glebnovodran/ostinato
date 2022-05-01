#!/bin/sh

GDB_CMDS=../gdb.cmds
DBG_OUT="" #"/dev/pts/5"
DBG_REDIR=${DBG_OUT:+">"}
echo "shell clear" > $GDB_CMDS
echo "echo \\\n~ \e[1m\e[33mWelcome to Ostinato debugging\e[0m ~\\\n\\\n" >> $GDB_CMDS
echo "python DBG_OUT=${DBG_OUT:+'$DBG_OUT' # }${DBG_OUT:-None}" >> $GDB_CMDS
echo "python\ndef dbg_out(msg, dbg=None):\n\tif not dbg: dbg=DBG_OUT\n\tif dbg and msg:\n\t\tos.system('echo \"'+msg+'\" > '+dbg)\nend" >> $GDB_CMDS
echo "break nxCore::dbg_break" >> $GDB_CMDS
echo "#break main" >> $GDB_CMDS

if [ -f "ostinato_dbg.py" ]; then
	echo "including py ext."
	echo "python\n" >> $GDB_CMDS
	cat ostinato_dbg.py >> $GDB_CMDS
	echo "\nend" >> $GDB_CMDS
fi

echo "run $* $DBG_REDIR $DBG_OUT" >> $GDB_CMDS
echo "tui enable" >> $GDB_CMDS
echo "#q" >> $GDB_CMDS
gdb ostinato -x $GDB_CMDS
