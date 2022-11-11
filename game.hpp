#ifndef _LEDUC_GAME_HEADER
#define _LEDUC_GAME_HEADER

#include <algorithm>
#include <random>
#include <functional>
#include <cassert>
#include <vector>
#include <map>

#define N_PLAYER 2 // leduc_poker has two player in each game

typedef enum leducCard_tag {
    Unknow = -1,
    RedK = 0,
    RedQ = 1,
    RedJ = 2,
    BlackK = 3,
    BlackQ = 4,
    BlackJ = 5
} leducCard;

typedef enum leducAction_tag {
    None  = -1,
    Call  = 0,
    Raise = 1,
    Fold  = 2,
} leducAction;

int valuefun(leducCard ac, leducCard desk);

struct TreeNode {
    TreeNode() : call(NULL), fold(NULL), raise(NULL) {}
    struct {
        leducAction lastAction;
        int         playerID;
        int         betting;
        bool        terminal;
    } info;
    TreeNode *raise;
    TreeNode *call;
    TreeNode *fold;
};

struct TwoMaxBetTree {
    TwoMaxBetTree(int plid_=0, int b_=1, int ramont_=2);
    ~TwoMaxBetTree();

    void buildTree();
    void freeTree(TreeNode *node);
    TreeNode *make_node(int bet, int player_id, int stop, int raise_time, leducAction action);

    TreeNode *root;
    int raise_amount;
};

typedef std::function<leducAction(leducCard*, TreeNode*, const std::vector<leducAction>&)> Strategy;

leducAction random_choice(leducCard *cards, TreeNode *state, const std::vector<leducAction> &history);
leducAction greedy_choice(leducCard *cards, TreeNode *state, const std::vector<leducAction> &history);
leducAction cfr(leducCard *cards, TreeNode *state, const std::vector<leducAction> &history);

/// @brief Player of LeducGame, only consider 2 players
class Player {
public:
    Player(const char* _name, int _chips=100);
    leducAction makeDecision(const leducCard desk, TreeNode *node, const std::vector<leducAction> &history);

    // properties
    std::string getName()           const { return name; }
    leducCard   showPrivateCard()   const { return privateCard; }
    int showChips()                 const { return chips; }
    int getID()                     const { return id; }
    bool isFolded()                 const { return folded; }

    // set properties
    void setFold()                    { folded = true; }
    void attachStrategy(Strategy st)  { strategy = st; }
    void setPrivateCard(leducCard pc) { privateCard = pc;}
    void setName(std::string s)       { name = s; }
    void reset()                      { privateCard  = leducCard::Unknow; folded = false; }

    // gaming
    int bet(int numbet, int *pot);
    int win(int *pot);

protected:
    int  chips;
    bool folded;
private:
    leducCard   privateCard;
    Strategy    strategy;
    std::string name;
    int id;
};

class CardMachine {
public:
    CardMachine();
    void reset();
    leducCard deal();
protected:
    void shuffleCards();
private:
    leducCard  cardpool[6]; // six cards
    leducCard *ptr_card;
};

TreeNode *two_bet_maximum(TwoMaxBetTree *tmbt, Player *pl, leducCard desk, std::vector<leducAction> &history,
                          int &betting, int raise, int *pot);
int check(Player *players, TreeNode *state, leducCard desk, int *pot);


// CFR strategy mapping
typedef std::map<std::string, std::tuple<double, double, double>> cfr_strategy;
static cfr_strategy cfr_str;
int load_cfr_strategy(const char *filename);

#endif