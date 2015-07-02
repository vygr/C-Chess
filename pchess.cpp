#include <iostream>
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <chrono>
#include <future>

//control paramaters
const int max_chess_moves     = 218;
const int max_ply             = 7;
const float max_time_per_move = 10000;

//piece values, in centipawns
const int king_value   = 20000;
const int queen_value  = 900;
const int rook_value   = 500;
const int bishop_value = 330;
const int knight_value = 320;
const int pawn_value   = 100;
const int mate_value   = king_value * 10;

//board square/piece types
const int white = 1;
const int empty = 0;
const int black = -1;

//piece capture actions, per vector
const int no_capture   = 0;
const int may_capture  = 1;
const int must_capture = 2;

//board is string of 64 chars
typedef std::string board;
typedef std::vector<board> boards;

//evaluation score and board combination
struct score_board
{
	int score;
	int bias;
	board brd;
};
typedef std::vector<score_board> score_boards;

//description of a pieces movement and capture action
struct move
{
	int dx;
	int dy;
	int length;
	int flag;
};
typedef std::vector<move> moves;

//description of a pieces check influence
struct vector
{
	int dx;
	int dy;
	int length;
};
typedef std::vector<vector> vectors;

//check test, array of pieces that must not be on this vectors from the king
struct test
{
	std::string pieces;
	vectors *vectors;
};
typedef std::vector<test> tests;

//map board square contents to piece type/colour
auto piece_type = std::map<char, int>{ \
	{'p', black}, {'r', black}, {'n', black}, {'b', black}, {'k', black}, {'q', black}, \
	{'P', white}, {'R', white}, {'N', white}, {'B', white}, {'K', white}, {'Q', white}, \
	{' ', empty}};

auto unicode_pieces = std::map<char, std::string>{ \
	{'p', "♟"}, {'r', "♜"}, {'n', "♞"}, {'b', "♝"}, {'k', "♚"}, {'q', "♛"}, \
	{'P', "♙"}, {'R', "♖"}, {'N', "♘"}, {'B', "♗"}, {'K', "♔"}, {'Q', "♕"}, \
	{' ', " "}};

//piece move vectors and capture actions
auto black_pawn_moves = moves{ \
	{0, 1, 0, no_capture}, {-1, 1, 1, must_capture}, {1, 1, 1, must_capture}};
auto white_pawn_moves = moves{ \
	{0, -1, 0, no_capture}, {-1, -1, 1, must_capture}, {1, -1, 1, must_capture}};
auto rook_moves = moves{ \
	{0, -1, 7, may_capture}, {-1, 0, 7, may_capture}, {0, 1, 7, may_capture}, {1, 0, 7, may_capture}};
auto bishop_moves = moves{ \
	{-1, -1, 7, may_capture}, {1, 1, 7, may_capture}, {-1, 1, 7, may_capture}, {1, -1, 7, may_capture}};
auto knight_moves = moves{ \
	{-2, 1, 1, may_capture}, {2, -1, 1, may_capture}, {2, 1, 1, may_capture}, {-2, -1, 1, may_capture}, \
	{-1, -2, 1, may_capture}, {-1, 2, 1, may_capture}, {1, -2, 1, may_capture}, {1, 2, 1, may_capture}};
auto queen_moves = moves{ \
	{0, -1, 7, may_capture}, {-1, 0, 7, may_capture}, {0, 1, 7, may_capture}, {1, 0, 7, may_capture}, \
	{-1, -1, 7, may_capture}, {1, 1, 7, may_capture}, {-1, 1, 7, may_capture}, {1, -1, 7, may_capture}};
auto king_moves = moves{ \
	{0, -1, 1, may_capture}, {-1, 0, 1, may_capture}, {0, 1, 1, may_capture}, {1, 0, 1, may_capture}, \
	{-1, -1, 1, may_capture}, {1, 1, 1, may_capture}, {-1, 1, 1, may_capture}, {1, -1, 1, may_capture}};

//map piece to its movement possibilities
auto moves_map = std::map<char, moves &>{ \
	{'p', black_pawn_moves}, {'P', white_pawn_moves}, {'R', rook_moves}, {'r', rook_moves}, {'B', bishop_moves}, {'b', bishop_moves}, \
	{'N', knight_moves}, {'n', knight_moves}, {'Q', queen_moves}, {'q', queen_moves}, {'K', king_moves}, {'k', king_moves}};

//piece check vectors, king is tested for being on these vectors for check tests
auto black_pawn_vectors = vectors{ \
	{-1, 1, 1}, {1, 1, 1}};
auto white_pawn_vectors = vectors{ \
	{-1, -1, 1}, {1, -1, 1}};
auto bishop_vectors = vectors{ \
	{-1, -1, 7}, {1, 1, 7}, {-1, 1, 7}, {1, -1, 7}};
auto rook_vectors = vectors{ \
	{0, -1, 7}, {-1, 0, 7}, {0, 1, 7}, {1, 0, 7}};
auto knight_vectors = vectors{ \
	{-1, -2, 1}, {-1, 2, 1}, {-2, -1, 1}, {-2, 1, 1}, {1, -2, 1}, {1, 2, 1}, {2, -1, 1}, {2, 1, 1}};
auto queen_vectors = vectors{ \
	{-1, -1, 7}, {1, 1, 7}, {-1, 1, 7}, {1, -1, 7}, {0, -1, 7}, {-1, 0, 7}, {0, 1, 7}, {1, 0, 7}};
auto king_vectors = vectors{ \
	{-1, -1, 1}, {1, 1, 1}, {-1, 1, 1}, {1, -1, 1}, {0, -1, 1}, {-1, 0, 1}, {0, 1, 1}, {1, 0, 1}};

//check tests, piece types given can not be on the vectors given
auto white_tests = tests{ \
	{"qb", &bishop_vectors}, {"qr", &rook_vectors}, {"n", &knight_vectors}, {"k", &king_vectors}, {"p", &white_pawn_vectors}};
auto black_tests = tests{ \
	{"QB", &bishop_vectors}, {"QR", &rook_vectors}, {"N", &knight_vectors}, {"K", &king_vectors}, {"P", &black_pawn_vectors}};

//map piece to black/white scores for board evaluation
auto piece_values = std::map<char, std::pair<int, int>>{ \
	{'k', {king_value, 0}}, {'K', {0, king_value}}, {'q', {queen_value, 0}}, {'Q', {0, queen_value}}, \
	{'r', {rook_value, 0}}, {'R', {0, rook_value}}, {'b', {bishop_value, 0}}, {'B', {0, bishop_value}}, \
	{'n', {knight_value, 0}}, {'N', {0, knight_value}}, {'p', {pawn_value, 0}}, {'P', {0, pawn_value}}};

//pawn values for position in board evaluation
auto pawn_position_values = std::array<int, 64>{ \
	0, 0, 0, 0, 0, 0, 0, 0, \
	50, 50, 50, 50, 50, 50, 50, 50, \
	10, 10, 20, 30, 30, 20, 10, 10, \
	5, 5, 10, 25, 25, 10, 5, 5, \
	0, 0, 0, 20, 20, 0, 0, 0, \
	5, -5, -10, 0, 0, -10, -5, 5, \
	5, 10, 10, -20, -20, 10, 10, 5, \
	0, 0, 0, 0, 0, 0, 0, 0};

//knight values for position in board evaluation
auto knight_position_values = std::array<int, 64>{ \
	-50, -40, -30, -30, -30, -30, -40, -50, \
	-40, -20, 0, 0, 0, 0, -20, -40, \
	-30, 0, 10, 15, 15, 10, 0, -30, \
	-30, 5, 15, 20, 20, 15, 5, -30, \
	-30, 0, 15, 20, 20, 15, 0, -30, \
	-30, 5, 10, 15, 15, 10, 5, -30, \
	-40, -20, 0, 5, 5, 0, -20, -40, \
	-50, -40, -30, -30, -30, -30, -40, -50};

//bishop values for position in board evaluation
auto bishop_position_values = std::array<int, 64>{ \
	-20, -10, -10, -10, -10, -10, -10, -20, \
	-10, 0, 0, 0, 0, 0, 0, -10, \
	-10, 0, 5, 10, 10, 5, 0, -10, \
	-10, 5, 5, 10, 10, 5, 5, -10, \
	-10, 0, 10, 10, 10, 10, 0, -10, \
	-10, 10, 10, 10, 10, 10, 10, -10, \
	-10, 5, 0, 0, 0, 0, 5, -10, \
	-20, -10, -10, -10, -10, -10, -10, -20};

//rook values for position in board evaluation
auto rook_position_values = std::array<int, 64>{ \
	0, 0, 0, 0, 0, 0, 0, 0, \
	5, 10, 10, 10, 10, 10, 10, 5, \
	-5, 0, 0, 0, 0, 0, 0, -5, \
	-5, 0, 0, 0, 0, 0, 0, -5, \
	-5, 0, 0, 0, 0, 0, 0, -5, \
	-5, 0, 0, 0, 0, 0, 0, -5, \
	-5, 0, 0, 0, 0, 0, 0, -5, \
	0, 0, 0, 5, 5, 0, 0, 0};

//queen values for position in board evaluation
auto queen_position_values = std::array<int, 64>{ \
	-20, -10, -10, -5, -5, -10, -10, -20, \
	-10, 0, 0, 0, 0, 0, 0, -10, \
	-10, 0, 5, 5, 5, 5, 0, -10, \
	-5, 0, 5, 5, 5, 5, 0, -5, \
	0, 0, 5, 5, 5, 5, 0, -5, \
	-10, 5, 5, 5, 5, 5, 0, -10, \
	-10, 0, 5, 0, 0, 0, 0, -10, \
	-20, -10, -10, -5, -5, -10, -10, -20};

//king values for position in board evaluation
auto king_position_values = std::array<int, 64>{ \
	-30, -40, -40, -50, -50, -40, -40, -30, \
	-30, -40, -40, -50, -50, -40, -40, -30, \
	-30, -40, -40, -50, -50, -40, -40, -30, \
	-30, -40, -40, -50, -50, -40, -40, -30, \
	-20, -30, -30, -40, -40, -30, -30, -20, \
	-10, -20, -20, -20, -20, -20, -20, -10, \
	20, 20, 0, 0, 0, 0, 20, 20, \
	20, 30, 10, 0, 0, 10, 30, 20};

//map piece to position value table
auto piece_positions = std::map<char, std::array<int, 64>&>{ \
	{'k', king_position_values}, {'K', king_position_values}, {'q', queen_position_values}, {'Q', queen_position_values}, \
	{'r', rook_position_values}, {'R', rook_position_values}, {'b', bishop_position_values}, {'B', bishop_position_values}, \
	{'n', knight_position_values}, {'N', knight_position_values}, {'p', pawn_position_values}, {'P', pawn_position_values}};

//clear screen
void cls()
{
	std::cout << "\033[H\033[2J";
}

//display board converting to unicode chess characters
void display_board(const board &brd)
{
	cls();
	std::cout << "\n  a   b   c   d   e   f   g   h\n";
	std::cout << "┏━━━┳━━━┳━━━┳━━━┳━━━┳━━━┳━━━┳━━━┓\n";
	for (auto row = 0; row < 8; ++row)
 	{
		for (auto col = 0; col < 8; ++col)
		{
			std::cout << "┃" << " " << unicode_pieces[brd[row*8+col]] << " ";
		}
		std::cout << "┃" << 8-row << "\n";
		if (row != 7)
		{
			std::cout << "┣━━━╋━━━╋━━━╋━━━╋━━━╋━━━╋━━━╋━━━┫\n";
		}
	}
	std::cout << "┗━━━┻━━━┻━━━┻━━━┻━━━┻━━━┻━━━┻━━━┛\n";
}

//generate all boards for a piece index and moves possibility
boards piece_moves(board brd, int index, const moves &moves)
{
	auto yield = boards{};
	yield.reserve(32);
	auto piece = brd[index];
	auto ptype = piece_type[piece];
	auto promote = std::string("qrbn");
	if (ptype == white)
 	{
		promote = "QRBN";
	}
	auto cx = index % 8;
	auto cy = index / 8;
	for (auto &move : moves)
 	{
		auto dx = move.dx;
		auto dy = move.dy;
		auto length = move.length;
		auto flag = move.flag;
		auto x = cx;
		auto y = cy;
		//special length for pawns so we can adjust for starting 2 hop
		if (length == 0)
		{
			length = 1;
			if (piece == 'p')
			{
				if (y == 1)
				{
					length = 2;
				}
			}
			else
			{
				if (y == 6)
				{
					length = 2;
				}
			}
		}
		while (length > 0)
		{
			x += dx;
			y += dy;
			length -= 1;
			if ((x < 0) || (x >= 8) || (y < 0) || (y >= 8))
			{
				//gone off the board
				break;
			}
			auto newindex = y*8 + x;
			auto newpiece = brd[newindex];
			auto newtype = piece_type[newpiece];
			if (newtype == ptype)
			{
				//hit one of our own piece type (black hit black etc)
				break;
			}
			if ((flag == no_capture) && (newtype != empty))
			{
				//not suposed to capture and not empty square
				break;
			}
			if ((flag == must_capture) && (newtype == empty))
			{
				//must capture and got empty square
				break;
			}
			brd[index] = ' ';
			if ((y == 0 || y == 7) && (piece == 'P' || piece == 'p'))
			{
				//try all the pawn promotion possibilities
				for (auto &promote_piece : promote)
				{
					brd[newindex] = promote_piece;
					yield.push_back(brd);
				}
			}
			else
			{
				//generate this as a possible move
				brd[newindex] = piece;
				yield.push_back(brd);
			}
			brd[index] = piece;
			brd[newindex] = newpiece;
			if ((flag == may_capture) && (newtype != empty))
			{
				//may capture and we did so !
				break;
			}
		}
	}
	return yield;
}
	
//generate all first hit pieces from index position along given vectors
std::string piece_scans(const board &brd, unsigned int index, const vectors &vectors)
{
	auto yield = std::string{};
	auto cx = index % 8;
	auto cy = index / 8;
	for (auto &vector : vectors)
 	{
		auto dx = vector.dx;
		auto dy = vector.dy;
		auto length = vector.length;
		int x = cx;
		int y = cy;
		while (length > 0)
		{
			x += dx;
			y += dy;
			length -= 1;
			if ((0 <= x) && (x < 8) && (0 <= y) && (y < 8))
			{
				//still on the board
				auto piece = brd[y*8+x];
				if (piece != ' ')
				{
					//not empty square so yield piece
					yield.push_back(piece);
					break;
				}
			}
		}
	}
	return yield;
}
	
//test if king of given colour is in check
bool in_check(const board &brd, int colour, std::size_t &king_index)
{
	auto king_piece = 'K';
	auto tests = white_tests;
	if (colour == black)
 	{
		//testing black king in check rather than white
		king_piece = 'k';
		tests = black_tests;
	}
	//find king index on board
	if (brd[king_index] != king_piece)
 	{
		king_index = brd.find(king_piece);
	}
	for (auto &test : tests)
 	{
		for (auto &piece : piece_scans(brd, static_cast<int>(king_index), *test.vectors))
		{
			if (test.pieces.find(piece) != std::string::npos)
			{
				//yes we found one of the pieces along a clear vector from king
				return true;
			}
		}
	}
	//not in check
	return false;
}
	
//generate all moves (boards) for the given colours turn filtering out position where king is in check
boards all_moves(const board &brd, int colour)
{
	auto yield = boards{};
	yield.reserve(max_chess_moves);
	//enumarate the board square by square
	std::size_t king_index = 0;
	for (auto index = 0; index < brd.length(); ++index)
	{
		auto piece = brd[index];
 		if (piece_type[piece] == colour)
		{
			//one of our pieces ! so gather all boards from possible moves of this piece
			for (auto &new_brd : piece_moves(brd, index, moves_map[piece]))
			{
				if (!in_check(new_brd, colour, king_index))
				{
					//on this board king is not in check
					yield.push_back(new_brd);
				}
			}
		}
	}
	return yield;
}

//evaluate (score) a board for the colour given
int evaluate(const board &brd, int colour)
{
	auto black_score = 0;
	auto white_score = 0;
	for (auto index = 0; index < brd.length(); ++index)
	{
		auto piece = brd[index];
		auto ptype = piece_type[piece];
		if (ptype != empty)
		{
			//add score for position on the board, near center, clear lines etc
			if (ptype == black)
			{
				black_score += piece_positions[piece][63-index];
			}
			else
			{
				white_score += piece_positions[piece][index];
			}
			//add score for piece type, queen, rook etc
			auto values = piece_values[piece];
			black_score += values.first;
			white_score += values.second;
		}
	}
	return (white_score - black_score) * colour;
}

//start of move time
auto start_time = std::chrono::high_resolution_clock::now();

//pvs alpha/beta pruning minmax search for given ply
int score(const board &brd, int colour, int alpha, int beta, int ply)
{
	if (ply == 0)
 	{
		return evaluate(brd, colour);
	}
	auto mate = true;
	for (auto &new_board : all_moves(brd, colour))
	{
		int value;
		if (!mate)
		{
			//not first child so null search window
			value = -score(new_board, -colour, -alpha-1, -alpha, ply-1);
			if (alpha < value && value < beta)
			{
				//failed high, so full re-search
				value = -score(new_board, -colour, -beta, -alpha, ply-1);
			}
		}
		else
		{
			value = -score(new_board, -colour, -beta, -alpha, ply-1);
		}
		mate = false;
		if (value >= mate_value)
		{
			//early return if mate
			return value;
		}
		if (value >= beta)
		{
			//fail hard beta cutoff
			return beta;
		}
		if (value > alpha)
		{
			alpha = value;
		}
		auto end_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> elapsed = end_time - start_time;
		if (elapsed.count() >= max_time_per_move)
		{
			//time has expired for this move
			break;
		}
	}
	if (mate)
 	{
		std::size_t king_index = 0;
		if (in_check(brd, colour, king_index))
		{
			//check mate
			return -mate_value - ply;
		}
		//stale mate
		return mate_value;
	}
	return alpha;
}

//pvs alpha/beta pruning minmax search for given ply
int first_ply_score(const board &brd, int colour, int ply)
{
	auto next_boards = score_boards{};
	next_boards.reserve(max_chess_moves);
	for (auto &brd : all_moves(brd, colour))
	{
		next_boards.push_back(score_board{evaluate(brd, colour), 0, brd});
	}
	auto mate = true;
	auto alpha = -mate_value*10;
	auto beta = mate_value*10;
	if (next_boards.size() != 0)
	{
		std::sort(begin(next_boards), end(next_boards), [&] (const auto &brd1, const auto &brd2)
		{
			return brd1.score > brd2.score;
		});
		for (auto &score_board : next_boards)
		{
			int value;
			if (!mate)
			{
				//not first child so null search window
				value = -score(score_board.brd, -colour, -alpha-1, -alpha, ply-1);
				if (alpha < value && value < beta)
				{
					//failed high, so full re-search
					value = -score(score_board.brd, -colour, -beta, -alpha, ply-1);
				}
			}
			else
			{
				value = -score(score_board.brd, -colour, -beta, -alpha, ply-1);
			}
			mate = false;
			if (value >= mate_value)
			{
				//early return if mate
				return value;
			}
			if (value >= beta)
			{
				//fail hard beta cutoff
				return beta;
			}
			if (value > alpha)
			{
				alpha = value;
			}
		}
	}
	if (mate)
 	{
		std::size_t king_index = 0;
		if (in_check(brd, colour, king_index))
		{
			//check mate
			return -mate_value - ply;
		}
		//stale mate
		return mate_value;
	}
	return alpha;
}

//best move for given board position for given colour
board best_move(const board &brd, int colour, const boards &history)
{
	//first ply of boards
	auto next_boards = score_boards{};
	next_boards.reserve(max_chess_moves);
	for (auto &brd : all_moves(brd, colour))
	{
		auto rep = std::count(begin(history), end(history), brd);
		next_boards.push_back(score_board{evaluate(brd, colour),
			static_cast<int>(-(rep * queen_value)), brd});
	}
	if (next_boards.size() == 0)
	{
		return std::string("");
	}
	
	//start move timer
	start_time = std::chrono::high_resolution_clock::now();
	for (auto ply = 1; ply <= max_ply; ++ply)
 	{
		//iterative deepening of ply so we allways have a best move to go with if the timer expires
		std::cout << "\nPly = " << ply << "\n";
		auto futures = std::vector<std::future<int>>{};
		futures.reserve(next_boards.size());
		for (auto &score_board: next_boards)
		{
			futures.push_back(std::async(std::launch::async, first_ply_score, score_board.brd, -colour, ply));
		}
		auto best_score = -mate_value*10;
		for (auto index = 0; index < next_boards.size(); ++index)
		{
			auto score_board = next_boards[index];
			score_board.score = futures[index].get() + score_board.bias;
			if (score_board.score > best_score)
			{
				//got a better board than last best
				best_score = score_board.score;
				std::cout << "*" << std::flush;
			}
			else
			{
				//just tick off another board
				std::cout << "." << std::flush;
			}
		}
		auto end_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> elapsed = end_time - start_time;
		if (elapsed.count() >= max_time_per_move)
		{
			//move timer expired
			break;
		}
		std::sort(begin(next_boards), end(next_boards), [&] (const auto &brd1, const auto &brd2)
		{
			return brd1.score > brd2.score;
		});
		if (best_score >= mate_value)
		{
			//don't look further ahead if we allready can force mate
			break;
		}
	}
	return next_boards[0].brd;
}

int main(int argc, const char * argv[])
{
	//setup first board, loop for white..black..white..black...
	auto game_start_time = std::chrono::high_resolution_clock::now();
	auto brd = board("rnbqkbnrpppppppp                                PPPPPPPPRNBQKBNR");
	//auto brd = board("rnb kbnrpppppppp                                PPPPPPPPRNBQKBNR");
	//auto brd = board("   r   kpB    pp  p  p    r p             PRRP  P P  P P  K     ");
	//auto brd = board(" k                         Q P     Q P  K                       ");
	//auto brd = board(" k                           P     Q P  K                       ");
	//auto brd = board("        p         k    p   rb         p      r              K   ");
	//auto brd = board("        p         k    p   r          p      r              K   ");
	//auto brd = board("                   k               K         Q                  ");
	//auto brd = board("    k     R        K                                            ");
	auto history = boards{};
	auto colour = white;
	display_board(brd);
	for (;;)
	{
		auto end_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end_time - game_start_time;
		std::cout << "Elapsed Time: " << elapsed.count() << "\n";
		if (colour == white)
		{
			std::cout << "\nWhite to move:\n";
		}
		else
		{
			std::cout << "\nBlack to move:\n";
		}
		auto new_brd = best_move(brd, colour, history);
		if (new_brd == "")
		{
			std::size_t king_index = 0;
			if (in_check(brd, colour, king_index))
			{
				std::cout << "\n** Checkmate **\n";
			}
			else
			{
				std::cout << "\n** Stalemate **\n";
			}
			break;
		}
		auto rep = std::count(begin(history), end(history), brd);
		if (rep >= 3)
		{
			std::cout << "\n** Draw **\n";
			break;
		}
		history.push_back(new_brd);
		for (auto i = 0; i < 3; ++i)
		{
			display_board(brd);
			std::this_thread::sleep_for(std::chrono::duration<float>(0.1));
			display_board(new_brd);
			std::this_thread::sleep_for(std::chrono::duration<float>(0.1));
		}
		colour = -colour;
		brd = new_brd;
	}
	return 0;
}
