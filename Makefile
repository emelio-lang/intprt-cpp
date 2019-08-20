%.cpp.cc: %.cpp
	python pp.py $<
	astyle $@ -Y
	rm *.orig

clean: 
	rm *.cpp.cc

build: Tokenizer/tokenizer.cpp Tokenizer/util.cpp util.cpp emelio.cpp parse.cpp.cc reduction.cpp.cc
	g++ Tokenizer/tokenizer.cpp Tokenizer/util.cpp util.cpp emelio.cpp  parse.cpp.cc reduction.cpp.cc -o emelio -g3 -std=c++17

clang: Tokenizer/tokenizer.cpp Tokenizer/util.cpp util.cpp emelio.cpp parse.cpp.cc reduction.cpp.cc
	clang Tokenizer/tokenizer.cpp Tokenizer/util.cpp util.cpp emelio.cpp parse.cpp.cc reduction.cpp.cc -o emelio -std=c++17

test:
	./utest.sh

run: emelio
	./emelio
