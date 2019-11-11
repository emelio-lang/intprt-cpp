./emelio c test.em > compiled/code.inc
pushd compiled
fasm lib.s
gcc lib.o -o a.out -m32 -g
./a.out
popd
