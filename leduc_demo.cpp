#include <iostream>
#include <cstdio>
#include <cstdarg>
#include "game.hpp"

/* compile with: g++ game.cpp leduc_demo.cpp -o leduc */

int main(int argc, char *argv[]) {
    const int numGames = 1000;
    const char *strategy_name[2];
    Strategy strategy[2];
    if (argc < 2) {
        printf("use default setting: CFR vs Greedy");
        load_cfr_strategy("offense.trained");
        load_cfr_strategy("defense.trained");
        strategy_name[0] = "CFR";
        strategy_name[1] = "Greedy";
        strategy[0] = cfr;
        strategy[1] = greedy_choice;
    }
    else if (argc == 3) {
        const char *pl[2] = {argv[1], argv[2]};
        for (int i = 0; i < 2; i++) {
            switch (pl[i][0]) {
                case 'c': {
                    printf("CFR");
                    strategy[i] = cfr;
                    load_cfr_strategy("offense.trained");
                    load_cfr_strategy("defense.trained");
                    strategy_name[i] = "CFR"; 
                    break;
                }
                case 'g': printf("Greedy"); strategy[i] = greedy_choice; strategy_name[i] = "Greedy"; break;
                case 'r': printf("Random"); strategy[i] = random_choice; strategy_name[i] = "Random"; break;
                default:
                    printf("error: unrecognized option %c. Supported 'c', 'g', or 'r'.", pl[i][0]);
            }
            if (i == 0) printf(" VS ");
        }
        putchar('\n');
    }
    else {
        puts("error: only 2 inputs are required.");
        return 0;
    }

    CardMachine cardMachine; // dealing the cards
    leducCard   desk;        // two cards on desk
    int pot = 0;             // chip pot

    // setup players
    Player players[2] = {Player(strategy_name[0], 3000),
                         Player(strategy_name[1], 3000)};
    players[0].attachStrategy(strategy[0]);
    players[1].attachStrategy(strategy[1]);

    // record the history of each game
    std::map<std::string, std::vector<int>> chip_history;
    if (players[0].getName() == players[1].getName()) {
        players[0].setName(players[0].getName() + "1");
        players[1].setName(players[1].getName() + "2");
    }


    /* playing games */
    for (int iGame = 0; iGame < numGames; iGame++) {
        // printf("%s first\n", players[0].getName().c_str());
        int betting = 1;
        desk = leducCard::Unknow;

        std::vector<leducAction> history({leducAction::None});
        
        // ante
        players[0].bet(betting, &pot);
        players[1].bet(betting, &pot);

        // private card
        players[0].setPrivateCard(cardMachine.deal());
        players[1].setPrivateCard(cardMachine.deal());

        // two-bet maximum rounds
        int first_player = 0;
        for (int r = 1; r <= 2; r++)
        {
            int raise = 2*r;
            TwoMaxBetTree tmbt(0, betting, raise);
            TreeNode *node = two_bet_maximum(&tmbt, players, desk, history, betting, raise, &pot);
            if (node->info.lastAction == leducAction::Fold || r == 2) {
                check(players, node, desk, &pot);
                break;
            }
            desk = cardMachine.deal();            // show public card on the desk
            history.push_back(leducAction::None); // indicator of end of a round of two-bet maximum
            first_player = node->info.playerID;
        }

        // record history
        chip_history[players[0].getName()].push_back(players[0].showChips());
        chip_history[players[1].getName()].push_back(players[1].showChips());

        // end of one game, reset the machine and players' card
        players[0].reset();
        players[1].reset();
        cardMachine.reset();

        // swap the order
        std::swap(players[0], players[1]);
    }


    printf("%s chips: %d\n", players[0].getName().c_str(), players[0].showChips());
    printf("%s chips: %d\n", players[1].getName().c_str(), players[1].showChips());

    // save result
    char filename[64] = {0};
    sprintf(filename, "%svs%s.csv", strategy_name[0], strategy_name[1]);
    FILE *fp = fopen(filename, "w");
    for (auto it: chip_history) {
        fprintf(fp, "%s,", it.first.c_str());
        for (auto chip: it.second) {
            fprintf(fp, "%d,", chip);
        }
        fputc('\n', fp);
    }
    fclose(fp);

    return 0;
}