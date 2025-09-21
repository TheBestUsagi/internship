savedcmd_/home/usagi/shared/mychar/mychar.mod := printf '%s\n'   mychar.o | awk '!x[$$0]++ { print("/home/usagi/shared/mychar/"$$0) }' > /home/usagi/shared/mychar/mychar.mod
