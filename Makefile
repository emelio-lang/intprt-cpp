%.cpp.cc: %.cpp
	python pp.py $<
#	astyle $@ -Y
#	rm *.orig

%.a: %.o
	ar r $@ $<
	ranlib $@

tkutil.o: Tokenizer/util.cpp
	g++ -c $< -g3 -o $@
tk.o: Tokenizer/tokenizer.cpp
	g++ -c $< -g3 -o $@
util.o: util.cpp emelio.h util.h
	g++ -c $< -g3 -o $@
reduction.o: reduction.cpp.cc emelio.h util.h notation.h
	g++ -c $< -g3 -o $@
parse.o: parse.cpp.cc emelio.h util.h
	g++ -c $< -g3 -o $@
notation.o: notation.cpp emelio.h util.h notation.h
	g++ -c $< -g3 -o $@
emelio.o: emelio.cpp emelio.h util.h
	g++ -c $< -g3 -o $@


OBJS = tkutil.o tk.o util.o parse.o notation.o reduction.o emelio.o 
SML_OBJS = tkutil.o tk.o  util.o parse.o emelio.o


clean: 
	rm *.cpp.cc

build: $(OBJS) emelio.h
	g++ -o emelio $(OBJS) -g3

sml-build: $(SML_OBJS) emelio.h
	g++ -o emelio $(SML_OBJS) -g3


clang: $(OBJS)
	clang -std=c++17 -o emelio $(OBJS)

test:
	./utest.sh

run: emelio
	./emelio
