set pagination off
set confirm off
set print pretty on
set logging file logs/gdb_bug.log
set logging overwrite on
set logging on

file ./app_bug
break print_array
run
finish
watch -l *(array+5)
continue

bt
info locals
print array
print ptr
print &array[0]
print &array[4]
print &array[5]
x/12wd array-1

quit
