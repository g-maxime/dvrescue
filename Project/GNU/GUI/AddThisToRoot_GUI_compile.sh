#! /bin/sh

#############################################################################
# Configure
Home=`pwd`

OS=$(uname -s)
if [ "$OS" = "Darwin" ]; then
    OS="mac"
elif [ "$(expr substr $OS 1 5)" = "Linux" ]; then
    OS="linux"
else
    OS="unix" # generic
fi


#############################################################################
# Setup for parallel builds
Zen_Make()
{
 if test -e /proc/stat; then
  numprocs=`egrep -c ^cpu[0-9]+ /proc/stat || :`
  if [ "$numprocs" = "0" ]; then
   numprocs=1
  fi
  make -s -j$numprocs
 else
  make
 fi
}

#############################################################################
# Setup for qmake
Q_Make()
{
    if qmake --version >/dev/null 2>&1 ; then
        qmake $*
    elif qmake-qt5 --version >/dev/null 2>&1 ; then
        qmake-qt5 $*
    elif qmake5 --version >/dev/null 2>&1 ; then
        qmake5 $*
    else
        echo qmake not found, please install Qt development package
        exit
    fi
}

#############################################################################
# qwt
if test -e qwt/qwt.pro; then
 cd qwt
 test -e Makefile && rm Makefile
 if test "$OS" != "mac"; then
     export QWT_STATIC=1
 fi
 export QWT_NO_SVG=1
 export QWT_NO_OPENGL=1
 export QWT_NO_DESIGNER=1
 Q_Make
 if test -e Makefile; then
  make clean
  Zen_Make
  if test -e lib/libqwt.a || test -e lib/qwt.framework; then
   echo qwt compiled
  else
   echo Problem while compiling qwt
   exit
  fi
 else
  echo Problem while configuring qwt
  exit
 fi
else
 echo qwt directory is not found
 exit
fi
cd $Home

#############################################################################
# dvrescue
if test -e dvrescue/Source/GUI/dvrescue/dvrescue.pro; then
 cd dvrescue/Source/GUI/dvrescue
 test -e Makefile && rm Makefile
 Q_Make
 if test -e Makefile; then
  make clean
  Zen_Make
  if test -e dvrescue/dvrescue || test -e dvrescue/dvrescue.app; then
   echo dvrescue compiled
  else
   echo Problem while compiling dvrescue
   exit
  fi
 else
  echo Problem while configuring dvrescue
  exit
 fi
else
 echo dvrescue directory is not found
 exit
fi
cd $Home

#############################################################################
# Going home
cd $Home
echo "dvrescue (GUI) executable is in Source/GUI/dvrescue"
