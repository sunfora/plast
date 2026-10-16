set -e

rm -rf colorize.so
make -s colorize.so

rm -rf libplast.so
GNU_MAKE_USE_HOOKS=1 /home/ivan/Projects/fork-open-source/make/make
