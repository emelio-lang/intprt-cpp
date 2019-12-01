%.cpp.cc: %.cpp
	python pp.py $<
	astyle $@ -Y
	rm *.orig

%.a: %.o
	ar r $@ $<
	ranlib $@

tkutil.o: Tokenizer/util.cpp
	g++ -pg -std=c++2a -c $< -g3 -o $@
tk.o: Tokenizer/tokenizer.cpp
	g++ -pg -std=c++2a -c $< -g3 -o $@
util.o: util.cpp emelio.h util.h
	g++ -pg -std=c++2a -c $< -g3 -o $@
reduction.o: reduction.cpp.cc emelio.h util.h notation.h
	g++ -pg --std=c++2a  -c $< -g3 -o $@
parse.o: parse.cpp.cc emelio.h util.h
	g++ -pg --std=c++2a  -c $< -g3 -o $@
notation.o: notation.cpp emelio.h util.h notation.h
	g++ -pg --std=c++2a  -c $< -g3 -o $@
transpile.o: transpile.cpp emelio.h util.h notation.h
	g++ -pg --std=c++2a  -c $< -g3 -o $@
codegen.o: codegen.cpp emelio.h util.h notation.h codegen.h
	g++ -pg --std=c++2a  -c $< -g3 -o $@
emelio.o: emelio.cpp emelio.h util.h codegen.h
	g++ -pg --std=c++2a  -c $< -g3 -o $@


OBJS = tkutil.o tk.o util.o parse.o notation.o reduction.o codegen.o emelio.o 
SML_OBJS = tkutil.o tk.o  util.o parse.o emelio.o

compile: emelio
	./emelio c test.em | gcc -lm -xc -Wall -o output -

comptest: emelio
	./emelio c test.em > compiled/vim.c
	astyle compiled/vim.c
	gcc -xc -lm -std=gnu11 -g compiled/vim.c -o compiled/a.out

compr: 
	./compiled/a.out

clean: 
	rm *.cpp.cc

build: $(OBJS) emelio.h
	g++ -pg -std=c++2a -o emelio $(OBJS) -g3

sml-build: $(SML_OBJS) emelio.h
	g++ -pg -std=c++2a -o emelio $(SML_OBJS) -g3


clang: $(OBJS)
	clang -std=c++17 -o emelio $(OBJS)

test:
	./utest.sh

run: emelio
	./emelio
