#! /bin/bash

# Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
#       Distributed under the 3-clause BSD License
#      (See accompanying file LICENSE.TXT or copy at
#  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

if test "$1" = "gcc" ; then 
  platform=gcc
  
  obj_ext=.o
  staticlib_ext=.a
  
  c_compiler=gcc
  c_compiler_opts=(-Wall -c -g -std=gnu9x -pthread -O0)
  
  cpp_compiler=g++
  cpp_compiler_opts=(-Wall -c -g -std=gnu++0x -pthread -O0)
  
  staticlib_linker=ar
  staticlib_linker_opts=()
  
  exe_linker=g++
  exe_linker_opts=(-Wall -g -std=gnu++0x -pthread -O0)
elif test "$1" = "mingw-w64" ; then 
  platform=mingw-w64
  
  obj_ext=.o
  staticlib_ext=.a
  
  c_compiler=x86_64-w64-mingw32-gcc
  c_compiler_opts=(-Wall -c -g -std=gnu9x -pthread -O0 -DNOMINMAX -DNTDDI_VERSION=0x06000000 -D_WIN32_WINNT=0x0600 -DWINVER=0x0600 -D_UNICODE -DUNICODE)
  
  cpp_compiler=x86_64-w64-mingw32-g++
  cpp_compiler_opts=(-Wall -c -g -std=gnu++0x -pthread -O0 -DNOMINMAX -DNTDDI_VERSION=0x06000000 -D_WIN32_WINNT=0x0600 -DWINVER=0x0600 -D_UNICODE -DUNICODE)
  
  staticlib_linker=x86_64-w64-mingw32-gcc-ar
  staticlib_linker_opts=()
  
  exe_linker=x86_64-w64-mingw32-g++
  exe_linker_opts=(-static -municode -Wall -g -std=gnu++0x -pthread -O0)
else
  echo "Invalid target platform \"$1\"."
  exit 1
fi

compile_c()
{
  objdir="$1"
  c="$2"
  shift
  shift
  opts=()
  if test $# -ne 0 ; then opts=("$@") ; fi
  obj="$objdir/`basename "$c" .c`$obj_ext"
  cmd=("$c_compiler" "${c_compiler_opts[@]}" "$c" -o "$obj" "${opts[@]}")

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
  if test $# -ne 0 ; then opts=("$@") ; fi
  obj="$objdir/`basename "$cpp" .cpp`$obj_ext"
  cmd=("$cpp_compiler" "${cpp_compiler_opts[@]}" "$cpp" -o "$obj" "${opts[@]}" )

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
    if echo "$obj" | grep -E '\'"$obj_ext"'$' > /dev/null ; then continue ; fi
    if echo "$obj" | grep -E '\'"$staticlib_ext"'$' > /dev/null ; then continue ; fi
    echo Bad obj "$obj"
    return 1
  done
  cmd=("$staticlib_linker" "${staticlib_linker_opts[@]}" crs "$lib" "${objs[@]}")

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
    if echo "$opt" | grep -E '\'"$obj_ext"'$' > /dev/null ; then continue ; fi
    if echo "$opt" | grep -E '\'"$staticlib_ext"'$' > /dev/null ; then continue ; fi
    echo Bad opt "$opt"
    return 1
  done
  cmd=("$exe_linker" "${exe_linker_opts[@]}" -o "$exe" "${opts[@]}")
  
  rm -f "$exe"
  echo 'XXXX'
  echo 'XXXX'
  echo "${cmd[@]}"
  mkdir -p "`dirname "$exe"`"
  "${cmd[@]}"
  x=$?
  return $x
}

compile_cs()
{
  objdir="$1"
  shift
  
  cs=()
  opts=()
  for x in "$@" ; do
    if echo "$x" | grep -E '^-'   > /dev/null ; then opts=("${opts[@]}" "$x") ; continue ; fi
    if echo "$x" | grep -E '\.c$' > /dev/null ; then cs=(  "${cs[@]}"   "$x") ; continue ; fi
  done
  
  for c in "${cs[@]}" ; do
    compile_c "$objdir" "$c" "${opts[@]}"
    x=$?
    if test $x -ne 0 ; then return 1 ; fi
  done
}

compile_cpps()
{
  objdir="$1"
  shift
  
  cpps=()
  opts=()
  for x in "$@" ; do
    if echo "$x" | grep -E '^-'     > /dev/null ; then opts=("${opts[@]}" "$x") ; continue ; fi
    if echo "$x" | grep -E '\.cpp$' > /dev/null ; then cpps=("${cpps[@]}" "$x") ; continue ; fi
  done
  
  for cpp in "${cpps[@]}" ; do
    compile_cpp "$objdir" "$cpp" "${opts[@]}"
    x=$?
    if test $x -ne 0 ; then return 1 ; fi
  done
}

##
##

#jbase
compile_cpps "tmp/$platform/jbase" jbase/*.cpp "-I${PWD}" 
x=$?; if test $x -ne 0; then exit 1; fi
link_staticlib "tmp/$platform/jbase/jbase$staticlib_ext" "tmp/$platform/jbase/"*$obj_ext
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: create junicode/generate-gciter-data-cfile.exe
compile_cpp "tmp/$platform/junicode" junicode/generate-gciter-data-cfile.cpp "-I${PWD}" 
x=$?; if test $x -ne 0; then exit 1; fi
link_exe "tmp/$platform/junicode/generate-gciter-data-cfile.exe" "tmp/$platform/junicode/generate-gciter-data-cfile$obj_ext"
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: create junicode/gciter-data-cfile.c
rm -f "tmp/$platform/junicode/gciter-data-cfile.c"
echo 'XXXX'
echo 'XXXX'
echo "./tmp/$platform/junicode/generate-gciter-data-cfile.exe > tmp/$platform/junicode/gciter-data-cfile.c"
( cd junicode ; "../tmp/$platform/junicode/generate-gciter-data-cfile.exe" > "../tmp/$platform/junicode/gciter-data-cfile.c" ; )
x=$?
if test $x -ne 0 ; then
  rm -f "tmp/$platform/junicode/gciter-data-cfile.c"
  exit 1
fi

#junicode: compile the generated c file
compile_c "tmp/$platform/junicode/" "tmp/$platform/junicode/gciter-data-cfile.c"
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: compile the other cpp files
cpps=(junicode/*.cpp)
cpps2=()
for cpp in "${cpps[@]}" ; do
  if echo "$cpp" | grep 'generate-gciter-data-cfile.cpp' > /dev/null ; then continue ; fi
  cpps2=("${cpps2[@]}" "$cpp")
done
compile_cpps "tmp/$platform/junicode" "${cpps2[@]}" "-I${PWD}" 
x=$?; if test $x -ne 0; then exit 1; fi

#junicode: link the obj files
objs=("tmp/$platform/junicode/"*$obj_ext)
objs2=()
for obj in "${objs[@]}" ; do
  if echo "$obj" | grep 'generate-gciter-data-cfile\'$obj_ext > /dev/null ; then continue ; fi
  objs2=("${objs2[@]}" "$obj")
done
link_staticlib "tmp/$platform/junicode/junicode$staticlib_ext" "${objs2[@]}"
x=$?; if test $x -ne 0; then exit 1; fi

#josutils
compile_cpps "tmp/$platform/josutils" josutils/*.cpp "-I${PWD}" 
x=$?; if test $x -ne 0; then exit 1; fi
link_staticlib "tmp/$platform/josutils/josutils$staticlib_ext" "tmp/$platform/josutils/"*$obj_ext
x=$?; if test $x -ne 0; then exit 1; fi

#iconv
if test "$platform" = "mingw-w64" ; then
  compile_cs  "tmp/$platform/libiconv"  libiconv/libiconv/*.c  \
        "-I${PWD}/libiconv/include/" \
        -DBUILDING_LIBICONV -DBUILDING_LIBCHARSET
  x=$?; if test $x -ne 0; then exit 1; fi
  link_staticlib "tmp/$platform/libiconv/libiconv$staticlib_ext" "tmp/$platform/libiconv/"*$obj_ext
  x=$?; if test $x -ne 0; then exit 1; fi
fi

#
linkagainst_iconv_opts=(-liconv)
if test "$platform" = "mingw-w64" ; then 
  linkagainst_iconv_opts=("-Ltmp/$platform/libiconv/" "${linkagainst_iconv_opts[@]}")
fi

#jjmake
compile_cpps "tmp/$platform/jjmake/" jjmake/*.cpp "-I${PWD}" 
x=$?; if test $x -ne 0; then exit 1; fi
link_exe  "bin/$platform/jjmake/jjmake"  "tmp/$platform/jjmake/"*$obj_ext  \
    "tmp/$platform/jbase/jbase$staticlib_ext" \
    "tmp/$platform/josutils/josutils$staticlib_ext" \
    "tmp/$platform/junicode/junicode$staticlib_ext" \
    "${linkagainst_iconv_opts[@]}"
x=$?; if test $x -ne 0; then exit 1; fi

#tests
compile_cpps "tmp/$platform/tests/" tests/*.cpp "-I${PWD}" 
x=$?; if test $x -ne 0; then exit 1; fi
link_exe  "bin/$platform/tests/tests"  "tmp/$platform/tests/"*$obj_ext  \
    "tmp/$platform/jbase/jbase$staticlib_ext" \
    "tmp/$platform/josutils/josutils$staticlib_ext" \
    "tmp/$platform/junicode/junicode$staticlib_ext" \
    "${linkagainst_iconv_opts[@]}"
x=$?; if test $x -ne 0; then exit 1; fi

#
echo Success
