#include "parameters.h"

const uint_fast8_t parameters::DB_SEARCH_MAX_REFUTE_WINS = 10;

const float parameters::PN_SEARCH_SELECTOR_CUT_ = 0.5f; // influences branching of the tree, the higher value 
														// the faster the search, but the search may become unreliable

#ifdef _DEBUG
const size_t parameters::PN_SEARCH_SIZE_LIMIT_ = 10000; // 10k, limits of the size of the tree
#elif TEST
const size_t parameters::PN_SEARCH_SIZE_LIMIT_ = 100000000; // 100M, limits of the size of the tree
#else
const size_t parameters::PN_SEARCH_SIZE_LIMIT_ = 300000; // 400k, limits of the size of the tree
#endif