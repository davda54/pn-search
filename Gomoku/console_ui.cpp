#include "console_ui.h"

#include <iostream>
#include <stdio.h>
#include <string>
#include <stdlib.h>


using namespace std;


// tries to read next move until the move is correct

// if the next move command is incorrect, the same function is called recursively
// with a warning message

coords console_ui::read_next_move(const simple_board& board, const string& message) {
	SetConsoleCursorPosition(hConsole, { 0, 35 });
	SetConsoleTextAttribute(hConsole, 7); // light gray

	clear_console(last_answer_length);
	cout << endl << "     " << message << endl << endl;
	cout << endl << "     next move: "; 
	clear_console(last_answer_length);

	SetConsoleTextAttribute(hConsole, 15); // default white colour
	string answer;
	cin >> answer;

	last_answer_length = answer.size();

	coords next_move;
	if (!coords::try_parse(answer, next_move)) return read_next_move(board, "\"" + answer + "\"" + " is not a correct input");
	if(!board.is_empty(next_move)) return read_next_move(board, "position \"" + answer + "\"" + " is already used");
	else return next_move;
}


// shows game over screen after one of the players wins

void console_ui::show_winning_screen(const simple_board& board, bool human_win) {
	render(board, coords::INCORRECT_POSITION, true);

	if (human_win) cout << endl << "     congratulation, you have defeated the undefeatable AI" << endl;
	else cout << endl << "     you have lost :(" << endl;

	cout << endl << "     press ENTER to start a new game";
	getchar(); getchar();
}


// deletes $number_of_chars characters in front of the cursor

void console_ui::clear_console(size_t number_of_chars) {
	for (int i = 0; i < number_of_chars; ++i)
		cout << " ";
	for (int i = 0; i < number_of_chars; ++i)
		cout << "\b";
}


// renders the board; when $last_move == INCORRECT_POSITION, don't highlight the last move

void console_ui::render(const simple_board& board, coords last_move, bool show_winning_line) {
	system("cls");

	SetConsoleTextAttribute(hConsole, 7); // light gray
	cout << endl << "     a b c d e f g h i j k l m n o" << endl;
	SetConsoleTextAttribute(hConsole, 8); // dark grey

	// first row: ┌─┬─┬─ ...

	cout << "    " << char(218) << char(196);
	for (int x = 0; x < 14; ++x)
		cout << char(194) << char(196);
	cout << char(191) << endl;
	
	for (int y = 0; y < constants::BOARD_SIZE; y++) {

		// even row: 12 │ │X│O│ │O ...

		SetConsoleTextAttribute(hConsole, 7); // light gray
		if (y <= 8) cout << "  " << y + 1 << " ";
		else cout << " " << y + 1 << " ";
		SetConsoleTextAttribute(hConsole, 8); // dark grey

		for (int x = 0; x < constants::BOARD_SIZE; ++x) {
			cout << char(179);

			bool highlight = false;
			// highlight the piece, if it was placed as a last move or if it is part of the winning line
			if (last_move == coords(x, y) || (show_winning_line && board.is_winning(coords(x, y))))
				highlight = true;

			switch (board.get_move(x, y)) {
				case NONE: cout << " "; break;
				case BLACK: SetConsoleTextAttribute(hConsole, highlight ? 0xC0 : 0x0C); cout << "X"; SetConsoleTextAttribute(hConsole, 8); break;
				case WHITE: SetConsoleTextAttribute(hConsole, highlight ? 0xB0 : 0x0B); cout << "O"; SetConsoleTextAttribute(hConsole, 8); break;
			}
		}
		cout << char(179) << endl;
		if (y == 14) break;

		// odd row:  ├─┼─┼─┼ ...

		cout << "    " << char(195) << char(196);
		for (int x = 0; x < 14; ++x)
			cout << char(197) << char(196);
		cout << char(180) << endl;		
	}

	// last row └─┴─┴─ ...

	cout << "    " << char(192) << char(196);
	for (int x = 0; x < 14; ++x)
		cout << char(193) << char(196);
	cout << char(217) << endl;

	SetConsoleTextAttribute(hConsole, 15); // default white colour
}