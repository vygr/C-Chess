all:		chess

chess:		chess.cpp
			clang++ -O3 -std=c++14 chess.cpp -o chess

clean:
			rm chess
