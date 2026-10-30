#include "buttons.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <thread>
#include <iostream>

#define BTN_A 27
#define BTN_B 22
#define BTN_UP 12
#define BTN_DOWN 16
#define BTN_LEFT 5
#define BTN_RIGHT 6

int chip_fd;
struct gpiohandle_data data_in;
int line_fd;

/* Buttons Layout
 *                     0
 * 5  4              3   2
 *                     1
 */

/* GPIOs
 *                     12
 * 27  22            5    6
 *                     16
 */

 /* Buttons are ACTIVE LOW */

Buttons::Buttons(GameState *game): game(game)
{
    // åbner file deskriptor for GPIO chip
    chip_fd = open("/dev/gpiochip0", O_RDONLY);
    // hvis åbning af GPIO chip fejler, udskrives en fejlmeddelelse og en undtagelse kastes
    if (chip_fd < 0) {
        perror("open()");
        throw std::runtime_error("Failed to open GPIO chip"); }
     
    // bruger denne struktur til at anmode om GPIO linjer
    struct gpiohandle_request req;
    // sætter alle felter i req til 0, så vi starter med en ren struktur
    memset(&req, 0, sizeof(req));

    // gemmer GPIO linjer for knapperne i en array
    req.lineoffsets[0] = BTN_A;
    req.lineoffsets[1] = BTN_B;
    req.lineoffsets[2] = BTN_UP;
    req.lineoffsets[3] = BTN_DOWN;
    req.lineoffsets[4] = BTN_LEFT; 
    req.lineoffsets[5] = BTN_RIGHT;
    
    // sætter pins til input 
    req.flags = GPIOHANDLE_REQUEST_INPUT;
    //  angiver at der er 6 forskellige GPIO linjer 
    req.lines = 6;  

    if (ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
        perror("ioctl(GPIO_GET_LINEHANDLE_IOCTL)");
        close(chip_fd);
        throw std::runtime_error("Kunne ikke anmode om GPIO-linjer");
    }
    // Gem line handle file descriptor, så den kan lukkes senere
    line_fd = req.fd;
}

Buttons::~Buttons()
{
    // Lukker GPIO line handle file descriptor
     if (close(chip_fd) < 0) {
        std::cerr << "Failed to close GPIO file descriptor" << std::endl;
    }
    if (close(line_fd) < 0) {
        std::cerr << "Failed to close GPIO line handle file descriptor" << std::endl;
    }
}

Vec2 Buttons::readMoveButtons()
{
    // tjekker at der er en gyldig line_fd, ellers returneres {0,0}
    if (data_in.values[2] == 0)      return {0,-1};
    else if (data_in.values[3] == 0) return {0,1};
    else if (data_in.values[4] == 0) return {-1,0};
    else if (data_in.values[5] == 0) return {1,0};
    else                             return {0,0};
    
}

class EdgeDetector{
private:
    int lastValue = 1;
public:
    EdgeDetector(int initialValue) : lastValue(initialValue) {};
    bool valueChanged(int value)
    {
        bool falling = (value != lastValue && value == 0);
        lastValue = value;
        return falling;
    }
};

void Buttons::buttonsThread()
{
    
    EdgeDetector shootDetector(1); // 1 = ikke trykket

    while (game->running)
    {

        // Opdater GPIO-input data buffer ved at kalde readGPIOs()
        readGPIOs();
        
        // Tjek skydeknappen BTN_A
        int currentShootState = data_in.values[0]; 

        // Dette sikrer, at selvom man holder knappen nede, trigges skuddet kun én gang ved tryk.
        if (shootDetector.valueChanged(currentShootState)) 
        {
            {
                std::lock_guard<std::mutex> lock(game->gameMutex);
                game->shootButtonLeft_flag = true;
            }
            game->shootButton_cv.notify_one();
        }

        // Kør tråden med et fast tidsinterval på 50 ms
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Buttons::readGPIOs()
{
    // opretter en struktur til at holde GPIO linje værdierne
    struct gpiohandle_data data;

    // læser værdierne for GPIO linjerne og gemmer dem i data strukturen
    if (ioctl(line_fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data) < 0) {
        perror("ioctl(GPIOHANDLE_GET_LINE_VALUES_IOCTL)");
    return;
    }

    // gemmmer de læste værdier i data_in, så de kan bruges andre steder i programmet
    data_in = data;
}
