#include <iostream>
#include <SDL2/SDL.h>
#include <ctime>   
#include <cstring>
#include <map>
#include <tuple>
#include <cstdlib>
#include <vector>

const int WIDTH = 960, HEIGHT = 640;
const int TILE_SIZE = 32;
const int MAP_WIDTH = 30; 
const int MAP_HEIGHT = 20;
const int VISIBLE_WIDTH = WIDTH / TILE_SIZE; // collumns visible on the screen
const int VISIBLE_HEIGHT = HEIGHT / TILE_SIZE;  // 15 rows visible on screen
const float playerSpeed = 10.0f;
const int zombieCount = 8;
const float zombieDelay = 4; // Seconds
const int zombieSpeed = 100;
std::map<std::tuple<int,int>,Uint64> flames;


struct Character {
    int x, y;
    int moveDelay;
    int counter;
    

    // Move character if enough time has passed
    void move(int targetX, int targetY) {
        if (counter >= moveDelay) {
            // Logic to move towards target
            if (x < targetX) x++;
            else if (x > targetX) x--;

            if (y < targetY) y++;
            else if (y > targetY) y--;
            
            counter = 0; // Reset counter after moving
        }
        counter++;
    }
};

struct Zombie : public Character {
    Zombie() : Character() {}
    Zombie(int startX, int startY, int delay, int startHealth) {
        x = startX;
        y = startY;
        moveDelay = delay;
        counter = 0;
    }
    // Move character if enough time has passed
    void move(int targetX, int targetY, int tilemap[][MAP_HEIGHT]) {
        int xChange = 0;
        int yChange = 0;

        if (counter >= moveDelay) {
            // Logic to move towards target
            if (x < targetX) xChange++;
            else if (x > targetX) xChange--;

            if (y < targetY) yChange++;
            else if (y > targetY) yChange--;

            // Making sure there is no barricade in the way and checking bounds
            int newX = x + xChange;
            int newY = y + yChange;
            
            if (newX >= 0 && newX < MAP_WIDTH && newY >= 0 && newY < MAP_HEIGHT) {
                if (tilemap[newX][newY] != 1) { // Assuming '1' is a barricade
                    x = newX;
                    y = newY;
                }
            }

            counter = 0; // Reset counter after moving
        }
        counter++;
    }
};
//zombie holding datastructure
std::vector<Zombie> zombies;

struct Player : public Character{
    int flameDuration = 3;

    Player() : Character(), flameDuration(3) {}
    Player( int startX, int startY, int delay, int startHealth) {
        x = startX;
        y = startY;
        moveDelay = delay;
        counter = 0; 
    }

    void flame(int tilemap[][MAP_HEIGHT]) {
        if (x > 2 && tilemap[x][y] ==3){
            tilemap[x-2][y] = 1;
            flames[std::make_tuple(x-2,y)] = SDL_GetTicks64();     
        }
    }
    void removeFlame(int tilemap[][MAP_HEIGHT], std::tuple<int,int> coords) {
        int removeX =std::get<0>(coords);
        int removeY =std::get<1>(coords);

        tilemap[removeX][removeY] = 3;
    }

    
    void checkFlame(int tilemap[][MAP_HEIGHT]) {
        Uint64 currentTime = SDL_GetTicks64();

        for (auto i = flames.begin(); i != flames.end(); ) {
            if (currentTime - i->second >= 4000) { 
                removeFlame(tilemap, i->first); 
                i = flames.erase(i);
            } else {
                i++;
            }
        }
    }

};

void pushZombie() {

    int zombieSpawn = rand() % 49;
    Zombie z;

    if (zombieSpawn > 19) {
        z = {zombieSpawn-19, 0, zombieSpeed, 0};
    } else {
        z = {0, zombieSpawn, zombieSpeed, 0};
    }

    zombies.push_back(z);
}


int main(int argc, char *argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create SDL Window
    SDL_Window *window = SDL_CreateWindow("Hello SDL WORLD", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create SDL Renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize Map
    SDL_Surface* tile_map_surface = SDL_LoadBMP("./src/images/tiles.bmp");
    if (!tile_map_surface) {
        std::cout << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create textures from surfaces
    SDL_Texture* tile_texture = SDL_CreateTextureFromSurface(renderer, tile_map_surface);
    SDL_FreeSurface(tile_map_surface);

    if (!tile_texture) {
        std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Procedural data structure for the tile map
    int tilemap[MAP_WIDTH][MAP_HEIGHT];

    // Procedural generation of tiles based on noise
    // tile types 1=gravel, 2=clay, 3=grass, 4=water
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            tilemap[x][y] = 3;
        }
    }

    // Populating the screen with tiles
    SDL_Rect tile[VISIBLE_WIDTH][VISIBLE_HEIGHT];

    for (int x = 0; x < VISIBLE_WIDTH; x++) {
        for (int y = 0; y < VISIBLE_HEIGHT; y++) {
            tile[x][y].x = x * TILE_SIZE;
            tile[x][y].y = y * TILE_SIZE;
            tile[x][y].w = TILE_SIZE;
            tile[x][y].h = TILE_SIZE;
        }
    }

    // The tiles below are the tiles to select from
    SDL_Rect select_tile_1 = {0, 0, TILE_SIZE, TILE_SIZE};
    SDL_Rect select_tile_2 = {TILE_SIZE, 0, TILE_SIZE, TILE_SIZE};
    SDL_Rect select_tile_3 = {0, TILE_SIZE, TILE_SIZE, TILE_SIZE};
    SDL_Rect select_tile_4 = {TILE_SIZE, TILE_SIZE, TILE_SIZE, TILE_SIZE};

    // Pointer to the keys
    const Uint8* pkeys = SDL_GetKeyboardState(NULL);

    //initial palyer position
    float precisePlayerX = VISIBLE_WIDTH / 2;
    float precisePlayerY = VISIBLE_HEIGHT / 2;
    bool playerMoved = false;

    // Time related variables
    Uint32 lastTime = SDL_GetTicks64();
    float deltaTime = 0;

    Uint32 zLastTime = SDL_GetTicks64();
    float zDeltaTime = 0;

    // Creating characters
    Player sean = {10, VISIBLE_HEIGHT - 2, 500, 0};
    Player player = { VISIBLE_WIDTH/2, VISIBLE_HEIGHT/2, 0, 0 };


    // Infinite loop running the game
    bool gameWon = false;
    bool running = true;
    SDL_Event windowEvent;


    // Main game loop
    while (running) {
        while (SDL_PollEvent(&windowEvent)) {
            if (windowEvent.type == SDL_QUIT) {
                running = false;
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Checking to see if sean reached the end of the map
        if (sean.x == VISIBLE_WIDTH - 1 && !gameWon) {
            gameWon = true; 
        }

        Uint32 zCurrentTime = SDL_GetTicks64();
        zDeltaTime = (zCurrentTime - zLastTime) / 1000.0f;
        

        // Handling Zombie generation
        if ((zombies.size() < zombieCount) && (zDeltaTime >= zombieDelay) && playerMoved) {
            pushZombie();
            zLastTime = zCurrentTime;
        }
        // Move bot to the right if player has moved
        if (playerMoved) { 

            sean.move(VISIBLE_WIDTH-1, VISIBLE_HEIGHT/2);

            //Moving the zombies
            for (auto it =zombies.begin(); it != zombies.end();) {
                it->move(sean.x,sean.y, tilemap);
                it++;
            }

        }

        // Calculating the time
        Uint32 currentTime = SDL_GetTicks64();
        deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Checking the map to see if flames need to be removed
        player.checkFlame(tilemap);

        if (pkeys[SDL_SCANCODE_SPACE]) {
            player.flame(tilemap);
        }

        if (pkeys[SDL_SCANCODE_UP]) {
            playerMoved = true;
            precisePlayerY -= playerSpeed * deltaTime;
        }

        if (pkeys[SDL_SCANCODE_DOWN]) {
            playerMoved = true;
            precisePlayerY += playerSpeed * deltaTime;
           
        }

        if (pkeys[SDL_SCANCODE_RIGHT]) {
            playerMoved = true;
            precisePlayerX += playerSpeed * deltaTime;
           
        }

        if (pkeys[SDL_SCANCODE_LEFT]) {
            playerMoved = true;
            precisePlayerX -= playerSpeed * deltaTime;
        }

        player.x = static_cast<int>(precisePlayerX);
        player.y = static_cast<int>(precisePlayerY);

        // keeping player in visible area
        if (player.x < 0) {
            player.x = 0;
        } else if (player.x >= VISIBLE_WIDTH-1) {
            player.x = VISIBLE_WIDTH - 1;
        }

        if (player.y < 0) {
            player.y = 0;
        } else if (player.y >= VISIBLE_HEIGHT) {
            player.y = VISIBLE_HEIGHT - 1;
        }


        // Render the tiles based on the offset and tilemap
        for (int x = 0; x < VISIBLE_WIDTH; x++) {
            for (int y = 0; y < VISIBLE_HEIGHT; y++) {
                switch (tilemap[x][y]) { // Adjust y-coordinate with offSetY
                    case 1:
                        SDL_RenderCopy(renderer, tile_texture, &select_tile_1, &tile[x][y]);
                        break;
                    case 2:
                        SDL_RenderCopy(renderer, tile_texture, &select_tile_2, &tile[x][y]);
                        break;
                    case 3:
                        SDL_RenderCopy(renderer, tile_texture, &select_tile_3, &tile[x][y]);
                        break;
                    case 4:
                        SDL_RenderCopy(renderer, tile_texture, &select_tile_4, &tile[x][y]);
                        break;
                }
            }
        }

        if (!gameWon) {
            // Draw the player as a green square
            SDL_Rect playerRect = {player.x * TILE_SIZE, player.y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red color for player
            SDL_RenderFillRect(renderer, &playerRect);



            // Drawing zombies and checking to see if they have reached sean
            for (auto it =zombies.begin(); it != zombies.end();) {
                if (it->x == sean.x && it->y == sean.y) running = false;

                SDL_Rect zombieRect = {it->x * TILE_SIZE, it->y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // green color for zombie
                SDL_RenderFillRect(renderer, &zombieRect);
                it++;
            }

            // Draw sean
            SDL_Rect seanRect = {sean.x * TILE_SIZE, sean.y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Red color for bot
            SDL_RenderFillRect(renderer, &seanRect);
        }

        // Present the drawn content
        SDL_RenderPresent(renderer);
    }

    // Clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(tile_texture);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
