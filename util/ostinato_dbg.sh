#!/bin/sh
GDB_CMDS=../gdb.cmds
DBG_OUT="" # " /dev/pts/3"
echo "echo \\\n~ Welcome to Ostinato debugging ~\\\n\\\n" > $GDB_CMDS
echo "break nxCore::dbg_break" >> $GDB_CMDS
echo "#break main" >> $GDB_CMDS
echo "run $* $DBG_OUT" >> $GDB_CMDS
echo "tui enable" >> $GDB_CMDS
echo "#q" >> $GDB_CMDS
gdb ostinato -x $GDB_CMDS
