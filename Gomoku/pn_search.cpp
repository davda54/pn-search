#include "pn_search.h"
#include "parameters.h"
#include "overlap_eval.h"
#include "threat_eval.h"
#include "db_search.h"

#include <iostream>


// informs the engine that a new move was placed on board

void pn_search::set_next_move(coords pos) {
	if (last_move_ != coords::INCORRECT_POSITION) {
		board_.place_move(last_move_, current_player_);
		base_selector_.full_update(board_, last_move_);
	}

	last_move_ = pos;
	change_player(current_player_);

	++move_number_;
}


// engine returns his best response to actual board, doesn't update current state

coords pn_search::get_response() {
#if defined(_DEBUG) || defined(TEST)
	auto start = std::chrono::high_resolution_clock::now();
#endif

	if (move_number_ == 0) return coords(7, 7);

	// create and initialize new root
	uint8_t root_threat = threat_eval::evaluate(board_.get_lines<5>(last_move_, current_player_)) & 0xC;
	root_ = pn_node(last_move_, pn_type::_OR, 1, 1, root_threat);

	search();

#if defined(_DEBUG) || defined(TEST)
	auto finish = std::chrono::high_resolution_clock::now();
	debug(finish - start);
#endif

	coords next_move = select_next_move();
	return next_move;
}


// starts developing the tree

void pn_search::search() {
	transposition_table_ = transposition_table();

	restart_state();
	select_most_proving();
	develop_node(root_);
	root_.update_ancestors();
	if (root_.children_size() == 1) {
		restart_state();
		return;
	}

	while (root_.proof() != 0 && root_.disproof() != 0 && root_.subtree_size() < parameters::PN_SEARCH_SIZE_LIMIT_) {
		restart_state();

		pn_node& mostProvingNode = select_most_proving();
		develop_node(mostProvingNode);
		mostProvingNode.update_ancestors();
	}
	restart_state();
}


// selects the best answer to the last moves according to the searched tree

coords pn_search::select_next_move() const {
	if (root_.proof() == 0) { // if value == TRUE, select the proving children
		size_t i = 0;
		while (root_.child(i)->proof() != 0) ++i;
		return root_.child(i)->position();
	}
	else if (root_.disproof() == 0) { // if value == FALSE, select the most developed node
		const pn_node* best_node = root_.child(0);
		for (size_t i = 1; i < root_.children_size(); ++i) {
			if (best_node->subtree_size() < root_.child(i)->subtree_size())
				best_node = root_.child(i);
		}
		return best_node->position();
	}
	else { // else select the most proving node
		const pn_node* best_node = root_.child(0);
		float best_ratio = float(root_.child(0)->proof()) / float(root_.child(0)->disproof());

		for (size_t i = 1; i < root_.children_size(); ++i) {
			float new_ratio = float(root_.child(i)->proof()) / float(root_.child(i)->disproof());
			if (best_ratio > new_ratio) {
				best_node = root_.child(i);
				best_ratio = new_ratio;
			}
		}
		return best_node->position();
	}
}


// selects the node that will most likely end the search

pn_node& pn_search::select_most_proving() {
	pn_node* selected_node = &root_;

	while (selected_node->children_size() > 0) { // go deeper the DAG until a leaf node is reached
		size_t i = 0;

		switch (selected_node->type()) {
			case pn_type::_OR:
				while (selected_node->child(i)->proof() != selected_node->proof()) ++i;
				break;
			case pn_type::_AND:
				while (selected_node->child(i)->disproof() != selected_node->disproof()) ++i;
				break;
		}

		update_state(selected_node->position());
		selected_node = selected_node->child(i);
	}

	update_state(selected_node->position());
	return *selected_node;
}


// adds children to the most-proving node

void pn_search::develop_node(pn_node& node) {

	// if the actual node is just a transposition of found_node, add its children and return

	auto found_node = transposition_table_.update(hash_.actual(), &node);
	if(found_node != nullptr) {
		for (auto&& child : found_node->children()) 
			node.add_child(child);
		return;
	}


	//  start dependency-based search and search for a winning move

	if (!(node.threat() & threat_eval::FOUR_ATTACK)) {
		db_search search(&board_);
		auto max_threat = (node.threat() & threat_eval::THREE_ATTACK) ? 1 : 2;
		auto winning_move = search.get_winning_move(player_, max_threat);
		if (winning_move) {
			switch (node.type()) {
			case pn_type::_OR: node.add_child(*winning_move, pn_type::_AND, 0, UINT32_MAX, UINT8_MAX); return;
			case pn_type::_AND: node.add_child(*winning_move, pn_type::_OR, UINT32_MAX, 0, UINT8_MAX); return;
			}
		}
	}


	// else select a few best moves according to selector's static evaluation

	selector_.assign_scores(board_, player_);
	auto score_cut = uint16_t(selector_.best_score() * parameters::PN_SEARCH_SELECTOR_CUT_);

	auto move = selector_.get_first();
	while (move.get_offset() != board_list_item::NULL_OFFSET) { // loop over all possible moves

		uint16_t score = move.score[player_ - 1];
		if (score >= score_cut) { // select only those with the best evaluation
			if (score == UINT16_MAX) { // if the move is proving, add it and return
				switch (node.type()) {
					case pn_type::_OR: node.add_child(move.position, pn_type::_AND, 0, UINT32_MAX, UINT8_MAX); return;
					case pn_type::_AND: node.add_child(move.position, pn_type::_OR, UINT32_MAX, 0, UINT8_MAX); return;
				}
			}

			node.add_child(move.position, node.type() == pn_type::_OR ? pn_type::_AND : pn_type::_OR, 1, 1, move.threats[player_ - 1] & 0xC);
		}

		move = selector_.get_item(move.previous);
	}
}


// updates actual state of the board

void pn_search::update_state(coords position) {
	board_.place_move(position, player_);
	position_history_.push(position);
	hash_.update(position, player_ - 1);
	selector_.shallow_update(board_, position);

	change_player(player_);
}


// restarts state of the board to the initial root value

void pn_search::restart_state() {
	while(!position_history_.empty()) {
		board_.delete_move(position_history_.top());
		position_history_.pop();
	}

	selector_ = base_selector_;
	player_ = current_player_;
	hash_.restart();
}


// debug-only function, it creates a command line subprogram that traverses the created search tree

#if defined(_DEBUG) || defined(TEST)
void pn_search::debug(std::chrono::duration<double, std::milli> duration) const {
	using namespace std;

	vector<const pn_node*> node_stack;

	const pn_node* actual = &root_;
	node_stack.push_back(actual);
	
	cout << endl << endl << "elapsed time: " << duration.count() << " ms";
	
	while (true) {

		cout << endl << endl << "path to actual: ";
		for (auto&& node : node_stack) cout << node->position().to_string() << " / ";
		cout << endl << endl;

		cout << "actual: proof = " << actual->proof() << ", disproof = " << actual->disproof() << ", threat = " << int(actual->threat()) << ", size = " << actual->subtree_size() << endl;
		for (auto&& child : actual->children())
			cout << "\t" << child->position().to_string() << ": proof = " << child->proof() << ", disproof = " << child->disproof() << ", threat = " << int(child->threat()) << ", size = " << child->subtree_size() << endl;
		
		while(true) {
			string command;
			cout << endl << "command: ";
			cin >> command;

			coords position;

			if(command == "x" || command == "exit") {
				return;
			}
			else if(command == "..") {
				node_stack.pop_back();
				actual = node_stack.back();
				break;
			} 
			else if(coords::try_parse(command, position)) {
				bool found = false;
				for (auto&& child : actual->children()) {
					if(child->position() == position) {
						actual = child.get();
						node_stack.push_back(actual);
						found = true;
						break;
					}
				}
				if (found) break;
			}
		}
	}
}
#endif