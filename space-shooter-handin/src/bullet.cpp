#include "bullet.hpp"
#include <mutex>
#include <condition_variable>
#include <chrono>

Bullet::Bullet(GameState *game, Buttons *buttons): game(game), buttons(buttons) {};

void Bullet::bulletInputThread() {
    auto shootBullet = [&]
    { game->bullets.push_back({game->shipPos.x + 1, game->shipPos.y + (shipHeight / 2)}); };

    while (game->running)
    {
        std::unique_lock<std::mutex> lock(game->gameMutex);
        // venter på at shootButtonLeft_flag bliver sat til ture eller at man stopper koden(!game->running)
        game->shootButton_cv.wait(lock, [&] {return !game->running || game->shootButtonLeft_flag; 
        });

        if(!game->running) break;
        
        // nulstiller flager så den er klar til næste tryk
        game->shootButtonLeft_flag = false; //resetter flaget
        
        // skyder 
        shootBullet();
    }


}

void Bullet::bulletUpdateThread() {
    while (game->running) {
        // Running at fixed speed (bullet movement)
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        {
            /* TODO: Update bullets (move right)
             * 1) Is locking required?
             * 2) For each game->bulles, move them 1 px horisontally
             * 3) Is sync required?
             */  
            std::lock_guard<std::mutex> lock(game->gameMutex);

            for (auto &b : game->bullets) {
                b.x += 1; // Move bullet right by 1 pixel
            }
            
            game->cv.notify_one();  // Notify gameControllerThread that enemies have moved
            
            // Delete bullet when out of screen
            for (auto b = game->bullets.begin(); b != game->bullets.end(); ) {
                if (b->x > displayWidth) {
                    b = game->bullets.erase(b);  // get next valid iterator (original b invalidated by erase)
                } else {
                    ++b;  // normal increment if no erase
                }
            }
        }
    }
}
