#!/bin/sh

cwd=`pwd`

top_srcdir=$cwd/../src/
install_dir=$top_srcdir/xul_gui/
#debug="--disable-debug --enable-m64"
debug="--enable-debug --enable-m64"
usepybr="--enable-python"

##

gecko_sdk_dir=$HOME/proj64/xulrunner/xulrunner-23.0.1-sdk
# gecko_sdk_dir=$HOME/proj/xulrunner/xulrunner-6.0-obj/dist
# gecko_sdk_dir=$HOME/proj/xulrunner/xulrunner-6.0-sdk

boost_dir=$HOME/proj64/boost/
fftw_dir=$HOME/proj64/
cgal_dir=$HOME/proj64/CGAL-3.8/
glew_dir=$HOME/proj64/glew

#######################

config_scr=../src/configure

if test ! -f $config_scr; then
    (
	cd ../src
	aclocal; glibtoolize --force; aclocal; autoheader; automake -a -W none; autoconf;
	cd js
	aclocal; autoheader; automake -a; autoconf;
    )	
fi

env CC="clang" \
CFLAGS="-O" \
CXX="clang++" \
CXXFLAGS="-O -std=c++11 -Wno-parentheses-equality -Wno-c++11-narrowing -Wno-extra-tokens -Wno-invalid-pp-token" \
$config_scr \
--disable-static \
--enable-shared \
--prefix=$install_dir \
$usepybr \
--with-xulrunner-sdk=$gecko_sdk_dir \
--with-boost=$boost_dir \
--with-fftw=$fftw_dir \
--with-cgal=$cgal_dir \
--with-glew=$glew_dir \
$debug

# --enable-npruntime
