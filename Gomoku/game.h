#ifndef GAME_H
#define GAME_H

#include "pn_search.h"
#include "console_ui.h"
#include "log_board.h"

#include <memory>


/* GAME
 * 
 * This is the main class that oversees the whole game.
 *
 * It can create a game between a human player and an AI that implements that class abstract_engine,
 * the game can be controlled via any UI that implements the class abstract_ui.
 *
 */

class game {
public:
	// creates a new game 
	explicit game(bool human_start) : ui_(std::make_unique<console_ui>()), engine_(std::make_unique<pn_search>()), 
									  current_player_(BLACK), human_on_move_(human_start), move_counter_(0) {}
	
	// initializes moves from a file
	void init(const std::string& filename);
	
	// starts the game
	void run();

private:
	log_board board_;
	std::unique_ptr<abstract_ui> ui_;
	std::unique_ptr<abstract_engine> engine_;
	figure current_player_;
	bool human_on_move_;
	coords last_move_;
	int move_counter_;

	void change_player() { abstract_engine::change_player(current_player_); human_on_move_ = !human_on_move_; }
};

#endif