#!/bin/sh
# $Id$

#### barf on errors
set -e

# may be used to force a certain automake-version e.g. 1.7
AMVERS=

# everybody who checks out the CVS wants the maintainer-mode to be enabled
# (should be off for source distributions, this should happen automatically)
DEFAULTCONFOPT="--enable-maintainer-mode"

# default values
DEBUG=1
OPTIM=0

usage () {
    echo "Usage: ./autogen.sh [options]"
    echo "  -i, --intel        use intel compiler"
    echo "  -g, --gnu          use gnu compiler (default)"
    echo "  --opts=FILE        use compiler-options from FILE"
    echo "  -d, --debug        switch debug-opts on"
    echo "  -n, --nodebug      switch debug-opts off"
    echo "  -o, --optim        switch optimization on"
    echo "  -h, --help         you already found this :)"
}

# no compiler set yet
COMPSET=0
for OPT in $* ; do

    set +e
    # stolen from configure...
    # when no option is set, this returns an error code
    arg=`expr "x$OPT" : 'x[^=]*=\(.*\)'`
    set -e

    case "$OPT" in
	-i|--intel)   . ./icc.opts ; COMPSET=1 ;;
	-g|--gnu)     . ./gcc.opts ; COMPSET=1 ;;
	--opts=*)
	    if [ -r $arg ] ; then
	      echo "reading options from $arg..."
	      . ./$arg ;
	      COMPSET=1;
	    else
	      echo "Cannot open compiler options file $arg!" ;
	      exit 1;
	    fi ;;
	-d|--debug)   DEBUG=1 ;;
	-n|--nodebug) DEBUG=0 ;;
	-o|--optim)   OPTIM=1 ;;
	-h|--help) usage ; exit 0 ;;
	# pass unknown opts to ./configure
	*) CONFOPT="$CONFOPT $OPT" ;;
    esac
done

# set default compiler
if [ "$COMPSET" != "1" ] ; then
    echo "No compiler set, using GNU compiler as default"
    . ./gcc.opts
fi

# create flags
COMPFLAGS="$FLAGS"

# maybe add debug flag
if [ "$DEBUG" = "1" ] ; then	
    COMPFLAGS="$COMPFLAGS $DEBUGFLAGS"
fi

# maybe add optimization flag
if [ "$OPTIM" = "1" ] ; then	
    COMPFLAGS="$COMPFLAGS $OPTIMFLAGS"
fi

# check if automake-version was set
if test "x$AMVERS" != x ; then
  echo Warning: explicitly using automake version $AMVERS
  # binaries are called automake-$AMVERS
  AMVERS="-$AMVERS"
fi

#### create all autotools-files

echo "--> libtoolize..."
# force to write new versions of files, otherwise upgrading libtools
# doesn't do anything...
libtoolize --force
# for plugin-stuff later: --ltdl

echo "--> aclocal..."
aclocal$AMVERS -I m4

echo "--> autoheader..."
autoheader

echo "--> automake..."
automake$AMVERS --add-missing

echo "--> autoconf..."
autoconf

#### start configure with special environment

export CC=$COMP
export CXX=$CXXCOMP
export CPP="$COMP -E"

export CFLAGS="$COMPFLAGS"
export CXXFLAGS="$COMPFLAGS"

./configure $DEFAULTCONFOPT $CONFOPT
