%.cpp.cc: %.cpp
	python pp.py $<
	astyle $@ -Y
	rm *.orig

%.a: %.o
	ar r $@ $<
	ranlib $@

tkutil.o: Tokenizer/util.cpp
	g++ -c $< -g3 -o $@
tk.o: Tokenizer/tokenizer.cpp
	g++ -c $< -g3 -o $@
util.o: util.cpp
	g++ -c $< -g3 -o $@
reduction.o: reduction.cpp.cc
	g++ -c $< -g3 -o $@
parse.o: parse.cpp.cc
	g++ -c $< -g3 -o $@
emelio.o: emelio.cpp
	g++ -c $< -g3 -o $@


OBJS = tkutil.o tk.o util.o parse.o reduction.o emelio.o


clean: 
	rm *.cpp.cc

build: $(OBJS) emelio.h
	g++ -o emelio $(OBJS) -g3

clang: $(OBJS)
	clang -std=c++17 -o emelio $(OBJS)

test:
	./utest.sh

run: emelio
	./emelio
