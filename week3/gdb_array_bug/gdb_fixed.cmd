set pagination off
set logging file logs/gdb_fixed.log
set logging overwrite on
set logging enabled on
file ./app_fix
run
quit
