make: Tokenizer/tokenizer.cpp Tokenizer/util.cpp util.cpp emelio.cpp parse.cpp
	g++ Tokenizer/tokenizer.cpp Tokenizer/util.cpp util.cpp parse.cpp emelio.cpp  -o emelio -g3

clang: Tokenizer/tokenizer.cpp Tokenizer/util.cpp util.cpp emelio.cpp parse.cpp
	clang Tokenizer/tokenizer.cpp Tokenizer/util.cpp util.cpp parse.cpp emelio.cpp  -o emelio

run: emelio
	./emelio
