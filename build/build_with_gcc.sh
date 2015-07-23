#! /bin/bash

compile_c()
{
  objdir="$1"
  c="$2"
  shift
  shift
  opts=()
  if test $# -ne 0 ; then opts="$@" ; fi
  obj="$objdir/`basename "$c" .c`.o"
  cmd=(gcc -Wall -c -g -std=gnu9x -pthread -O0 "-I${PWD}" "$c" -o "$obj" "${opts[@]}" )

  rm -f "$obj"
  echo 'XXXX'
  echo 'XXXX'
  echo "${cmd[@]}"
  mkdir -p "$objdir"
  "${cmd[@]}"
  x=$?
  return $x
}

compile_cpp()
{
  objdir="$1"
  cpp="$2"
  shift
  shift
  opts=()
  if test $# -ne 0 ; then opts="$@" ; fi
  obj="$objdir/`basename "$cpp" .cpp`.o"
  cmd=(g++ -Wall -c -g -std=gnu++0x -pthread -O0 "-I${PWD}" "$cpp" -o "$obj" "${opts[@]}" )

  rm -f "$obj"
  echo 'XXXX'
  echo 'XXXX'
  echo "${cmd[@]}"
  mkdir -p "$objdir"
  "${cmd[@]}"
  x=$?
  return $x
}

link_staticlib()
{
  lib="$1"
  shift
  objs=()
  if test $# -ne 0 ; then objs=("$@") ; fi
  for obj in "${objs[@]}" ; do
    if echo "$obj" | grep -E '\.o$' > /dev/null ; then continue ; fi
    if echo "$obj" | grep -E '\.a$' > /dev/null ; then continue ; fi
    echo Bad obj "$obj"
    return 1
  done
  cmd=(ar crs "$lib" "${objs[@]}")

  rm -f "$lib"
  echo 'XXXX'
  echo 'XXXX'
  echo "${cmd[@]}"
  mkdir -p "`dirname "$lib"`"
  "${cmd[@]}"
  x=$?
  return $x
}

link_exe()
{
  exe="$1"
  shift
  opts=()
  if test $# -ne 0 ; then opts=("$@") ; fi
  for opt in "${opts[@]}" ; do
    if echo "$opt" | grep -E '^-' > /dev/null ; then continue ; fi
    if echo "$opt" | grep -E '\.o$' > /dev/null ; then continue ; fi
    if echo "$opt" | grep -E '\.a$' > /dev/null ; then continue ; fi
    echo Bad opt "$opt"
    return 1
  done
  cmd=(g++ -Wall -g -std=gnu++0x -pthread -O0 -o "$exe" "${opts[@]}")
  
  rm -f "$exe"
  echo 'XXXX'
  echo 'XXXX'
  echo "${cmd[@]}"
  mkdir -p "`dirname "$exe"`"
  "${cmd[@]}"
  x=$?
  return $x
}

compile_cpps()
{
  objdir="$1"
  shift
  cpps=("$@")
  for cpp in "${cpps[@]}" ; do
    compile_cpp "$objdir" "$cpp"
    x=$?
    if test $x -ne 0 ; then return 1 ; fi
  done
}

##
##

platform=gcc

#jbase
compile_cpps tmp/$platform/jbase/ jbase/*.cpp
x=$?; if test $x -ne 0; then exit 1; fi
link_staticlib tmp/$platform/jbase/jbase.a tmp/$platform/jbase/*.o
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: create junicode/generate-gciter-data-cfile.exe
compile_cpp tmp/$platform/junicode/ junicode/generate-gciter-data-cfile.cpp
x=$?; if test $x -ne 0; then exit 1; fi
link_exe tmp/$platform/junicode/generate-gciter-data-cfile.exe tmp/$platform/junicode/generate-gciter-data-cfile.o
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: create junicode/gciter-data-cfile.c
rm -f "tmp/$platform/junicode/gciter-data-cfile.c"
echo 'XXXX'
echo 'XXXX'
echo "./tmp/$platform/junicode/generate-gciter-data-cfile.exe > tmp/$platform/junicode/gciter-data-cfile.c"
( cd junicode ; ../tmp/$platform/junicode/generate-gciter-data-cfile.exe > ../tmp/$platform/junicode/gciter-data-cfile.c ; )
x=$?
if test $x -ne 0 ; then
  rm -f "tmp/$platform/junicode/gciter-data-cfile.c"
  exit 1
fi

#junicode: compile the generated c file
compile_c tmp/$platform/junicode/ tmp/$platform/junicode/gciter-data-cfile.c
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: compile the other cpp files
cpps=(junicode/*.cpp)
cpps2=()
for cpp in "${cpps[@]}" ; do
  if echo "$cpp" | grep 'generate-gciter-data-cfile.cpp' > /dev/null ; then continue ; fi
  cpps2=("${cpps2[@]}" "$cpp")
done
compile_cpps tmp/$platform/junicode/ "${cpps2[@]}"
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: link the obj files
objs=(tmp/$platform/junicode/*.o)
objs2=()
for obj in "${objs[@]}" ; do
  if echo "$obj" | grep 'generate-gciter-data-cfile.o' > /dev/null ; then continue ; fi
  objs2=("${objs2[@]}" "$obj")
done
link_staticlib tmp/$platform/junicode/junicode.a "${objs2[@]}"
x=$?; if test $x -ne 0; then exit 1; fi

#josutils
compile_cpps tmp/$platform/josutils/ josutils/*.cpp
x=$?; if test $x -ne 0; then exit 1; fi
link_staticlib tmp/$platform/josutils/josutils.a tmp/$platform/josutils/*.o
x=$?; if test $x -ne 0; then exit 1; fi

#jjmake
compile_cpps tmp/$platform/jjmake/ jjmake/*.cpp
x=$?; if test $x -ne 0; then exit 1; fi
link_exe bin/$platform/jjmake/jjmake tmp/$platform/jjmake/*.o tmp/$platform/jbase/jbase.a tmp/$platform/junicode/junicode.a tmp/$platform/josutils/josutils.a
x=$?; if test $x -ne 0; then exit 1; fi

#
echo Success
