#include "bit_board.h"


using namespace std;


// default initialization of lines to be empty - i.e. contain only figure::NONE pieces

bit_board::bit_board() {
	for (int i = 0; i < SIZE; ++i) horizontal_[i] = 0x3FFFFFFF; // initialize each line to contain 15 NONE pieces - i.e. 15x the number 0x3
	for (int i = 0; i < SIZE; ++i) vertical_[i] = 0x3FFFFFFF; // initialize each line to contain 15 NONE pieces - i.e. 15x the number 0x3
	for (int i = 0; i < SIZE; ++i) {

		/* the pattern, that is being filled here via ugly bit manipulations is the following: (for the $diagonal_ array, $anti_diagonal is horizontally flipped)
		 *
		 *     X
		 *    XX
		 *   XXX
		 *  XXXX
		 *  XXX
		 *  XX
		 *  X
		 *
		 *  this represents the diagonal cuts of the board (a1, b1-a2, c1-b2-a3, ...)
		 */ 

		diagonal_[2*SIZE - 2 - i] = anti_diagonal_[i] = (1 << (2 * (i + 1))) - 1;
		diagonal_[i] = anti_diagonal_[2*SIZE - 2 - i] = ((1 << (2 * (i + 1))) - 1) << (2*(SIZE - 1 - i));
	}
}


// initialization of masks that allow operations on the board
// ugly bit operations are done trivially by following the definitions (in header file) of each mask

bit_board::bit_board_masks bit_board::masks_;
bit_board::bit_board_masks::bit_board_masks() {
	for (uint32_t i = 0; i < SIZE; ++i) {
		delete_mask[i] = 3 << (i * 2); // shifting ..00000 11 00000..

		figure_mask[OUTSIDE][i] = ~((uint32_t(OUTSIDE) ^ 3) << (i * 2)); // shifting ...11111 fig 11111...
		figure_mask[WHITE][i] = ~((uint32_t(WHITE) ^ 3) << (i * 2));
		figure_mask[BLACK][i] = ~((uint32_t(BLACK) ^ 3) << (i * 2));
		figure_mask[NONE][i] = ~((~uint32_t(NONE) ^ 3) << (i * 2));
	}

	for (uint32_t radius = 0; radius < 8; ++radius) {
		// this makes the following pattern: 11, 11.11.11, 11.11.11.11.11, ...
		line_mask[radius] = (1 << (2 * (2 * radius + 1))) - 1;
	}
}


// places move on each of the arrays by applying figure masks

void bit_board::place_move(coords pos, figure fig) {
#ifdef _DEBUG
	if (pos.x < 0 || pos.x >= SIZE || pos.y < 0 || pos.y >= SIZE) throw runtime_error("position " + pos.to_string() + " is out of board");
	debug_board_.place_move(pos, fig);
#endif

	horizontal_[pos.y] &= masks_.figure_mask[fig][pos.x];
	vertical_[pos.x] &= masks_.figure_mask[fig][pos.y];
	diagonal_[pos.y - pos.x + SIZE - 1] &= masks_.figure_mask[fig][pos.x];
	anti_diagonal_[pos.x + pos.y] &= masks_.figure_mask[fig][pos.y];
}


// deletes move from each of the arrays by applying delete masks

void bit_board::delete_move(coords pos) {
#ifdef _DEBUG
	if (pos.x < 0 || pos.x >= SIZE || pos.y < 0 || pos.y >= SIZE) throw runtime_error("position " + pos.to_string() + " is out of board");
	debug_board_.delete_move(pos);
#endif

	horizontal_[pos.y] |= masks_.delete_mask[pos.x];
	vertical_[pos.x] |= masks_.delete_mask[pos.y];
	diagonal_[pos.y - pos.x + SIZE - 1] |= masks_.delete_mask[pos.x];
	anti_diagonal_[pos.x + pos.y] |= masks_.delete_mask[pos.y];
}