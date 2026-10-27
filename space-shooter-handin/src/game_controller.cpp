#include "game_controller.hpp"
#include <mutex>
#include <chrono>
#include <iostream>

GameController::GameController(GameState *game): game(game) {};

void GameController::gameControllerThread() {
    while (game->running) {
        std::unique_lock<std::mutex> lock(game->gameMutex);
        
        // Wait for enemy or bullet for max 25 ms
        game->cv.wait_for(lock, std::chrono::milliseconds(125));

        // tjekker om spillet stadig kører, hvis ikke, brydes løkken
        if (!game->running) break;

        // Collision detection
        for (auto b = game->bullets.begin(); b != game->bullets.end();) {
            bool hit = false;
            for (auto e = game->enemies.begin(); e != game->enemies.end();) {
                if ((b->x == e->x) && (abs(b->y - e->y)<enemyHeight)) {
                    e = game->enemies.erase(e);
                    game->score++;
                    hit = true;
                }
                else
                {
                    ++e;
                }
            }
            if (hit) 
            {
                b = game->bullets.erase(b);
            } 
            else 
            {
                ++b;
            }
        }

        //Game over condition
        for (auto &e : game->enemies) {
            // hvis en enemy når venstre side af skærmen, sættes game->running til false, hvilket stopper spillet 
            if (e.x <= 0) {
             game->running = false;
             }
         }

    }
}
