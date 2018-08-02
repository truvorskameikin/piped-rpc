#include <iostream>
#include "rpc-client.h"

int main(int argc, char* argv[]) {
  rpc::Client client("localhost");

  std::function<void(int32_t, int)> sum_result_func = [](int32_t request_id, int result) {
    std::cout << "Request " << request_id << " finished, sum: " << result << std::endl;
  };

  while (true) {
    std::cout << "What are you doing to do?" << std::endl;
    std::cout << "1: Create player" << std::endl;
    std::cout << "2: Add money to player" << std::endl;
    std::cout << "3: Get player money" << std::endl;
    std::cout << "4: Calculate sum of money of all players" << std::endl;

    int method = 0;
    std::cin >> method;

    if (method < 0 || method > 5)
      continue;

    switch (method) {
    case 1:
      {
        std::cout << "Creating player..." << std::endl;

        std::cout << "Name: ";

        std::string name;
        std::cin >> name;

        std::cout << "Money: ";

        int money = 0;
        std::cin >> money;

        std::cout << "Player created: " << client.CreatePlayer(name, money) << std::endl;
      }

      break;

    case 2:
      {
        std::cout << "Adding money to player..." << std::endl;

        std::cout << "Player: ";

        int player_id = 0;
        std::cin >> player_id;

        std::cout << "Money: ";

        int money = 0;
        std::cin >> money;

        client.AddMoneyToPlayer(player_id, money);
      }
      break;

    case 3:
      {
        std::cout << "Getting player money..." << std::endl;

        std::cout << "Player: ";

        int player_id = 0;
        std::cin >> player_id;

        std::cout << "Player's " << player_id << " money: " << client.GetPlayerMoney(player_id) << std::endl;
      }
      break;

    case 4:
      std::cout << "Starting async request to calculate sum of money of all players, request id: " << client.SumMoneyForAllPlayersAsync(sum_result_func) << std::endl;
      break;
    }
  }

  return 0;
}
