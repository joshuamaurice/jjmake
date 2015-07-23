#! /bin/bash

# TODO run from the root workspace directory

compile_c()
{
  c="$1"
  shift
  opts=()
  if test $# -ne 0 ; then opts="$@" ; fi
  obj="`dirname "$c"`"/"`basename "$c" .c`".o
  cmd=(gcc -Wall -c -g -std=gnu9x -pthread -O0 "-I${PWD}" "$c" -o "$obj" "${opts[@]}" )

  rm -f "$obj"
  echo "${cmd[@]}"
  "${cmd[@]}"
  x=$?
  echo ''
  return $x
}

compile_cpp()
{
  cpp="$1"
  shift
  opts=()
  if test $# -ne 0 ; then opts="$@" ; fi
  obj="`dirname "$cpp"`"/"`basename "$cpp" .cpp`".o
  cmd=(g++ -Wall -c -g -std=gnu++0x -pthread -O0 "-I${PWD}" "$cpp" -o "$obj" "${opts[@]}" )

  rm -f "$obj"
  echo "${cmd[@]}"
  "${cmd[@]}"
  x=$?
  echo ''
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

  rm -f "$lib"
  cmd=(ar crs "$lib" "${objs[@]}")
  echo "${cmd[@]}"
  "${cmd[@]}"
  x=$?
  echo ''
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

  rm -f "$exe"
  cmd=(g++ -Wall -g -std=gnu++0x -pthread -O0 -o "$exe" "${opts[@]}")
  echo "${cmd[@]}"
  "${cmd[@]}"
  x=$?
  echo ''
  return $x
}

compile_cpps()
{
  cpps=("$@")
  for cpp in "${cpps[@]}" ; do
    compile_cpp "$cpp"
    x=$?
    if test $x -ne 0 ; then return 1 ; fi
  done
}

##
##


#jbase
compile_cpps jbase/*.cpp
x=$?; if test $x -ne 0; then exit 1; fi
link_staticlib jbase/jbase.a jbase/*.o
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: create junicode/generate-gciter-data-cfile.exe
compile_cpp junicode/generate-gciter-data-cfile.cpp
x=$?; if test $x -ne 0; then exit 1; fi
link_exe junicode/generate-gciter-data-cfile.exe junicode/generate-gciter-data-cfile.o
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: create junicode/gciter-data-cfile.c
rm -f junicode/gciter-data-cfile.c
echo './generate-gciter-data-cfile.exe > gciter-data-cfile.c'
( cd junicode ; ./generate-gciter-data-cfile.exe > gciter-data-cfile.c ; )
x=$?
if test $x -ne 0 ; then
  rm junicode/gciter-data-cfile.c
  exit 1
fi

#junicode: compile the generated c file
compile_c junicode/gciter-data-cfile.c
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: compile the other cpp files
cpps=(junicode/*.cpp)
cpps2=()
for cpp in "${cpps[@]}" ; do
  if echo "$cpp" | grep 'generate-gciter-data-cfile.cpp' > /dev/null ; then continue ; fi
  cpps2=("${cpps2[@]}" "$cpp")
done
compile_cpps "${cpps2[@]}"
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: link the obj files
objs=(junicode/*.o)
objs2=()
for obj in "${objs[@]}" ; do
  if echo "$obj" | grep 'generate-gciter-data-cfile.o' > /dev/null ; then continue ; fi
  objs2=("${objs2[@]}" "$obj")
done
link_staticlib junicode/junicode.a "${objs2[@]}"
x=$?; if test $x -ne 0; then exit 1; fi

#josutils
compile_cpps josutils/*.cpp
x=$?; if test $x -ne 0; then exit 1; fi
link_staticlib josutils/josutils.a josutils/*.o
x=$?; if test $x -ne 0; then exit 1; fi

#jjmake
compile_cpps jjmake/*.cpp
x=$?; if test $x -ne 0; then exit 1; fi
link_exe jjmake/jjmake jjmake/*.o jbase/jbase.a junicode/junicode.a josutils/josutils.a
x=$?; if test $x -ne 0; then exit 1; fi

#
echo Success

