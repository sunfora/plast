set -e

rm -rf colorize.so
DEBUG=1 make -s colorize.so

rm -rf libplast.so
GNU_MAKE_USE_HOOKS=1 gdb /home/ivan/Projects/fork-open-source/make/make
