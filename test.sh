set -e

gcc -shared -fPIC -O2 -g3 -o colorize.so colorize.c
rm -rf libplast.so 
/home/ivan/Projects/fork-open-source/make/make
