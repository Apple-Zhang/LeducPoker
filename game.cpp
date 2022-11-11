#include "game.hpp"

int valuefun(leducCard ac, leducCard desk) {
    int value[6] = {3, 2, 1, 3, 2, 1};
    int ac_index = (int)ac;
    int dk_index = (int)desk;
    
    assert(ac_index >= 0 && ac_index < 6);
    assert(dk_index >= 0 && dk_index < 6);
    if (value[ac_index] == value[dk_index]) { // same suit
        return value[ac_index] * value[dk_index] * 10;
    }
    else {
        return value[ac_index] + value[dk_index];
    }
}

/* =========== Tree class =========== */
TwoMaxBetTree::TwoMaxBetTree(int plid_, int b_, int ramont_) {
    raise_amount = ramont_;
    root = new TreeNode;
    root->info.betting = b_;            // betting
    root->info.playerID = plid_;        // who make decision in this node
    root->info.terminal = false;
    root->info.lastAction = leducAction::None;
    buildTree();
}

TwoMaxBetTree::~TwoMaxBetTree() {
    freeTree(root);
}

void
TwoMaxBetTree::buildTree() {
    root->fold  = NULL;
    root->call  = make_node(root->info.betting, 1-root->info.playerID, false, 0, leducAction::Call);
    root->raise = make_node(root->info.betting + raise_amount, 1-root->info.playerID, false, 1, leducAction::Raise);
}

TreeNode*
TwoMaxBetTree::make_node(int bet, int player_id, int stop, int raise_time, leducAction action) {
    TreeNode *node = new TreeNode;
    node->info.betting = bet;
    node->info.playerID = player_id;
    node->info.terminal = stop;
    node->info.lastAction = action;

    if (!stop) {
        node->call = make_node(bet, 1-player_id, true, raise_time, leducAction::Call);
        
        // fold
        if (raise_time > 0) {
            node->fold = make_node(bet, 1-player_id, true, raise_time, leducAction::Fold);
        }

        // raise
        if (raise_time < 2) {
            node->raise = make_node(bet + this->raise_amount, 1-player_id, false, raise_time+1, leducAction::Raise);
        }
    }
    return node;
}

void
TwoMaxBetTree::freeTree(TreeNode *node) {
    if (node) { 
        freeTree(node->raise);
        freeTree(node->call);
        freeTree(node->fold);
        delete node;
    }
}

/// @brief random strategy
/// @param cards all observation cards
/// @param state state of game
/// @param chips chips you have
/// @return decision
leducAction random_choice(leducCard *cards, TreeNode *state, const std::vector<leducAction> &history) {
    /* set random seed */
    srand(time(NULL) + (int)cards[0]);

    std::vector<leducAction> options;

    // push the potential options
    if (state->call)  options.push_back(leducAction::Call);
    if (state->raise) options.push_back(leducAction::Raise);
    if (state->fold)  options.push_back(leducAction::Fold);

    int ind = std::rand() % options.size();

    return options[ind];
}

/// @brief greedy strategy
/// @param cards all observation cards
/// @param state state of game
/// @param chips chips you have
/// @return decision
leducAction greedy_choice(leducCard *cards, TreeNode *state, const std::vector<leducAction> &history) {
    if (cards[1] == leducCard::Unknow) {
        switch (cards[0]) {
            case leducCard::BlackK:
            case leducCard::RedK:
                if (state->raise) return leducAction::Raise;
            case leducCard::BlackQ:
            case leducCard::RedQ:
                if (state->call)  return leducAction::Call;
            case leducCard::BlackJ:
            case leducCard::RedJ:
                if (state->fold)  return leducAction::Fold;
                else              return leducAction::Call;
            default:              return leducAction::Call;
        }
    }
    else {
        int value = valuefun(cards[0], cards[1]);
        if (value > 10) {
            if (state->raise) return leducAction::Raise;
            else              return leducAction::Call;
        }
        else if (value >= 4) {
            if (state->call)  return leducAction::Call;
        }
        else {
            if (state->fold)  return leducAction::Fold;
            else              return leducAction::Call;
        }
    }
    return leducAction::None;
}


/// @brief CFR
/// @param cards all observation cards
/// @param state state of game
/// @param chips chips you have
/// @return decision
leducAction cfr(leducCard *cards, TreeNode *state, const std::vector<leducAction> &history) {
    // static uniform random engine
    static std::random_device rd;
    static std::uniform_real_distribution<double> rng(0.0, 1.0);

    // generate string for query of the policy
    std::string query;
    for (int i = 0; i < 2; i++) {
        switch (cards[i]) {
            case leducCard::BlackJ:
            case leducCard::RedJ: query += "J"; break;
            case leducCard::BlackQ:
            case leducCard::RedQ: query += "Q"; break;
            case leducCard::BlackK:
            case leducCard::RedK: query += "K"; break;
        }
    }
    query += ":";
    for (auto h: history) {
        switch (h) {
            case leducAction::None:  query += "/"; break;
            case leducAction::Call:  query += "c"; break;
            case leducAction::Raise: query += "r"; break;
            default: break;
        }
    }
    query += ":";

    // obtain the policy
    assert(cfr_str.find(query) != cfr_str.end());
    std::tuple<double, double, double> policy = cfr_str[query];

    // decide what to do with random number.
    double rand_val = rng(rd);
    if (rand_val <= std::get<0>(policy)) {
        return leducAction::Raise;
    }
    else if (rand_val <= std::get<0>(policy) + std::get<1>(policy)) {
        return leducAction::Call;
    }
    else {
        return leducAction::Fold;
    }
}

/* =========== Player class =========== */
Player::Player(const char* _name, int _chips) : chips(_chips), name(_name) {
    this->reset(); 
}

leducAction
Player::makeDecision(const leducCard desk, TreeNode *node, const std::vector<leducAction> &history) {
    assert(!folded);
    leducCard observation[2] = {privateCard, desk};
    return strategy(observation, node, history);
}

int
Player::bet(int numbet, int *pot) {
    chips -= numbet;
    *pot += numbet;
    return numbet;
}

int
Player::win(int *pot) {
    chips += *pot;
    *pot = 0;
    return chips;
}


/* =========== Card Machine =========== */
CardMachine::CardMachine() {
    cardpool[0] = leducCard::RedJ;
    cardpool[1] = leducCard::RedQ;
    cardpool[2] = leducCard::RedK;
    cardpool[3] = leducCard::BlackJ;
    cardpool[4] = leducCard::BlackQ;
    cardpool[5] = leducCard::BlackK;
    reset();
}

void
CardMachine::reset() {
    shuffleCards();
    ptr_card = &cardpool[0];
}

leducCard
CardMachine::deal() {
    assert(ptr_card < cardpool + 6);
    return *ptr_card++;
}

void
CardMachine::shuffleCards() {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(cardpool, cardpool+6, rng);
}


TreeNode *two_bet_maximum(TwoMaxBetTree *tmbt, Player *pl, leducCard desk, std::vector<leducAction> &history,
                          int &betting, int raise, int *pot) {
    TreeNode *state = tmbt->root;
    leducAction action = leducAction::None;

    int i = 0;
    while (state->call || state->fold || state->raise) { // check leaf 
        // make decision
        action = pl[i].makeDecision(desk, state, history);
        history.push_back(action);

        // take action
        switch (action) {
            case leducAction::Call:  {
                pl[i].bet(betting, pot);
                state = state->call;
                break;
            }
            case leducAction::Fold: {
                pl[i].setFold();
                state = state->fold;
                break;
            }  
            case leducAction::Raise: {
                betting += tmbt->raise_amount;
                pl[i].bet(betting, pot);
                state = state->raise;
                break;
            }
            default:
                assert(0);
        }
        assert(i == 1 || i == 0); i = 1-i; // change the player
        assert(state);
    }
    return state;
}

int check(Player *players, TreeNode *state, leducCard desk, int *pot) {
    if (state->info.lastAction == leducAction::Fold) {
        return players[state->info.playerID].win(pot);
    }
    
    leducCard plA_card = players[0].showPrivateCard();
    leducCard plB_card = players[1].showPrivateCard();
    if (valuefun(plA_card, desk) > valuefun(plB_card, desk)) {
        return players[0].win(pot);
    }
    else if (valuefun(plA_card, desk) < valuefun(plB_card, desk)) {
        return players[1].win(pot);
    }
    else {
        return -1;
    }
}

int load_cfr_strategy(const char *filename) {
    char line[1024] = {0};

    FILE *fp = fopen(filename, "rt");
    assert(fp);
    while (fgets(line, 1024, fp)) {
        if (line[0] != '#') {
            char cards[64] = {0};
            double raise, call, fold;

            // check valid input
            assert(sscanf(line, "%s%lf%lf%lf", cards, &raise, &call, &fold) == 4);

            cfr_str[std::string(cards)] = std::tuple<double, double, double>(raise, call, fold);
        }
    }
    fclose(fp);
    return 0;
}
