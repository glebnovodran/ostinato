#!/bin/sh
GDB_CMDS=../gdb.cmds
DBG_OUT="" #"/dev/pts/5"
DBG_REDIR=${DBG_OUT:+">"}
echo "echo \\\n~ Welcome to Ostinato debugging ~\\\n\\\n" > $GDB_CMDS
echo "python DBG_OUT=${DBG_OUT:+'$DBG_OUT' # }${DBG_OUT:-None}" >> $GDB_CMDS
echo "python\ndef dbg_out(msg):\n\tif DBG_OUT and msg:\n\t\tos.system('echo \"'+msg+'\" > '+DBG_OUT)\nend" >> $GDB_CMDS
echo "break nxCore::dbg_break" >> $GDB_CMDS
echo "#break main" >> $GDB_CMDS
echo "run $* $DBG_REDIR $DBG_OUT" >> $GDB_CMDS
echo "tui enable" >> $GDB_CMDS
echo "#q" >> $GDB_CMDS
gdb ostinato -x $GDB_CMDS
