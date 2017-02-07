#!/bin/bash

if [ $# -lt 1 ]
then
        echo "Usage : $0 challenge #"
        exit
fi

case "$1" in

0)  echo "Just touch .hello_there"
    touch .hello_there
    ;;
1)  echo  "We shall not write !"
    chmod -w .hello_there 
    ;;
2)  echo  "Sending a SIGCONT signal"
    ps -Af | grep "riddle"
    echo "now send the signal"
    ;;
3) echo  "42 of course "
   echo "export ANSWER=42"
   ;;
4) echo  "magic_mirror"
   mkfifo magic_mirror
   ;;
5) ./chal05
   ;;
6) ./chal06
   ;;
7) echo  "hard links"
   ln .hello_there .hey_there
   ;;
8) ./chal08
   ;;
9) echo "now run strace ./riddle"
   ./chal09
   ;;
10) ./chal10
   ;;
11) ./chal11
   ;;
12) ./chal12
   ;;
13) ./chal14
   ;;
14) ./chal14
   ;;


*) echo "Solutions not found yet"
   ;;
esac

