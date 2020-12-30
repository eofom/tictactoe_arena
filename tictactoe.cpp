#include "lib.h"
#include "eofom_player.h"

int main() {
    auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<std::unique_ptr<BasePlayer>> players;
    players.emplace_back(std::move(std::make_unique<BasePlayer>()));
    players.emplace_back(std::move(std::make_unique<EofomPlayer>()));
    playTictactoe(players, 100000);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeToComplete = (endTime - startTime);
    std::cout << timeToComplete.count() << "s\n";
    return 0;
}
