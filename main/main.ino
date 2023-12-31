#include <M5StickCPlus.h>
#include "FastLED.h"
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <cstring>

#define Neopixel_PIN    32
#define NUM_LEDS        90
#define GRID_W          4
#define GRID_H          15

// Colors
#define BUN_COL 0x28dee8
#define KETCHUP_COL 0xff0000
#define MUSTARD_COL 0xffea00
#define BEEF_COL 0x1d1405
#define CHEESE_COL 0xe5a40d
#define BURGER_SAUCE_COL 0x534822
#define BACON_COL 0xba1f1f
#define SALAD_COL 0x12ba17
#define ONION_COL 0x6f1d61
#define MAYO_COL 0xeadf69
#define BBQ_COL 0x3e2a17

#define UNOCCUPIED_COL 0x00
#define LOSE_COL 0xff0000

#define UNOCCUPIED 0
#define OCCUPIED 1
#define FALLING_TILE 2

#define KEY_A 1
#define KEY_B 1 << 1
#define KEY_C 1 << 2

// Settings
#define DELTA_TIME_DEFAULT 250
#define DEATH_ANIMATION_DELAY 1000
#define WIN_ANIMATION_DELAY 1000
#define SPEED_FACTOR 0.93
#define KEY_C_PIN 0

#define PLAYER_COUNT 2
#define STACK_WIDTH 1

std::unordered_map<uint8_t, int> layerToColor = {
    {14, BUN_COL},
    {13, KETCHUP_COL},
    {12, MUSTARD_COL},
    {11, BEEF_COL},
    {10, CHEESE_COL},
    {9, BUN_COL},
    {8, BBQ_COL},
    {7, BEEF_COL},
    {6, BACON_COL},
    {5, SALAD_COL},
    {4, KETCHUP_COL}, // tomato
    {3, ONION_COL},
    {2, MAYO_COL},
    {1, BURGER_SAUCE_COL},
    {0, BUN_COL}
};

enum GameState {
    // In this state while in menu
    MENU,

    // In this state when the game is waiting for the user to place the layer
    // I.e the layer should shift left to right in this state
    WAIT_ON_INPUT,

    // Right after a stack is placed this state is active for as long as
    // There are blocks that needs to fall to the ground
    TILES_FALLING,

    // A player loses but the game is not finished
    LOSE,

    // When all players are dead
    GAMEOVER,

    // A player won the game: show a win animation and go back to menu
    WIN,
};

struct PlayerData {
    // Stores all the tiles the player has a block placed.
    // UNOCCUPIED: No block is placed at that position
    // OCCUPIED: a block is placed at the position
    // FALLING_TILE: the block at that position is falling
    uint8_t occupiedTiles[GRID_H][GRID_W];

    // The working grid of occupied tiles. Will be written to occupiedTiles after
    // every tick, too avoid interference when editing and reading in same tick
    uint8_t nextOccupiedTiles[GRID_H][GRID_W];

    // The layer this player has reached
    uint8_t activeLayer = GRID_H - 1;

    int simSpeed = DELTA_TIME_DEFAULT;

    bool lost = false;
};


PlayerData playerInfos[PLAYER_COUNT];

int activePlayerIdx = 0;

// The index position of the head on the active layer
// Used to keep track on the currently placing stack
// in the WAIT_ON_INPUT state
int stackWiggleHeadPos = 0;

CRGB leds[NUM_LEDS];
GameState gameState = GameState::MENU;

// Bitmap representing if a button have been clicked within the last update tick
// idx 001: Btn A
// idx 010: Btn B
// idx 100: Btn C (custom button)
int inputBitmap = 0;

// 2D array which translates a pixel coordinate to the index of the LED to
// light up in the physical LED grid. Ie. grid[1][3] will be the LED idx of
// the LED representing the (1,3) pixel coordinate
uint8_t grid[GRID_H][GRID_W];

CRGB drawBuffer[GRID_H][GRID_W] = {
    {0xff0000, 0x0, 0x0, 0x0},
    {0xff0000, 0x0, 0x0, 0xff0000},
    {0xff0000, 0xff0000, 0xff0000, 0xff0000},
    {},
    {0x0, 0xff0000, 0xff0000, 0xff0000},
    {0xff0000, 0x0, 0xff0000, 0x0},
    {0x0, 0xff0000, 0xff0000, 0xff0000},
    {},
    {0xff0000, 0x0, 0x0, 0xff0000},
    {0xff0000, 0x0, 0xff0000, 0xff0000},
    {0xff0000, 0xff0000, 0x0, 0xff0000},
};

void pollInput() {
    if (M5.BtnA.wasReleased())
        inputBitmap |= KEY_A;

    if (M5.BtnB.wasReleased())
        inputBitmap |= KEY_B;

    // Custom button
    if (digitalRead(KEY_C_PIN) == LOW)
        inputBitmap |= KEY_C;
}

void writeOccupiedToDrawBuffer() {
    for (int row = 0; row < GRID_H; row++) {
        for (int col = 0; col < GRID_W; col++) {
            drawBuffer[row][col] = layerToColor[row] * playerInfos[activePlayerIdx].occupiedTiles[row][col];
        }
    }
}


void gameLoop(void* params) {
    for (;;) {
        switch (gameState) {
            case MENU:
                inMenu();
                break;

            case WAIT_ON_INPUT:
                waitOnInput();
                break;

            case TILES_FALLING:
                tilesFalling();
                break;

            case LOSE:
                loosing();
                break;

            case GAMEOVER:
                gameOver();
                break;

            case WIN:
                winning();
                break;

            default:
                break;
        }
        showDrawBuffer();
        writeOccupiedToDrawBuffer();
        

        // Reset input from this tick - accumulate input events until next tick
        inputBitmap = 0;

        // Copy old working grid to active grid
        std::memcpy(
                playerInfos[activePlayerIdx].occupiedTiles,
                playerInfos[activePlayerIdx].nextOccupiedTiles,
                GRID_H * GRID_W);

        // FIXME: subtract time took to tick game
        int simSpeed = playerInfos[activePlayerIdx].simSpeed;
        vTaskDelay(simSpeed / portTICK_PERIOD_MS);
    }
}

void setup() {
    // Calculate pixel coordinate to LED index grid
    grid[0][0] = 27;
    grid[0][1] = 33;
    grid[0][2] = 69;
    grid[0][3] = 75;
    for (int row = 1; row < GRID_H; row++) {
        grid[row][0] = grid[row - 1][0] - 1;
        grid[row][1] = grid[row - 1][1] + 1;
        grid[row][2] = grid[row - 1][2] - 1;
        grid[row][3] = grid[row - 1][3] + 1;
    }

    // Setup custom button
    pinMode(KEY_C_PIN, INPUT);

    // Setup m5 stuff, see https://docs.m5stack.com/en/api/stickc/system_m5stickc
    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(20, 2);
    M5.Lcd.println("Miku's burgeria");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(15, 35);

    // Initalize LED array
    FastLED.addLeds<WS2811, Neopixel_PIN, GRB>(leds, NUM_LEDS)
        .setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(64);

    // Register gameloop
    xTaskCreatePinnedToCore(gameLoop, "Game Loop", 8128, NULL, 1, NULL, 1);

    // Init draw buffer
    for (int i = 0; i < GRID_H; i++) {
        for (int j = 0; j < GRID_W; j++) {
            drawBuffer[i][j] = UNOCCUPIED_COL;
        }
    }
}

void loop() {
    // Update key states
    M5.update();
    pollInput();
}

void inMenu() {
    // TODO: show idle animation

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Click button to start game...");

    CRGB menuBuffer[GRID_H][GRID_W] = {
        {0x0, 0x0, 0xff0000, 0xff0000},
        {0x0, 0x0, 0xff0000, 0xff0000},
        {0x0, 0x0, 0x0, 0xff0000},
        {},
        {0x0, 0x0, 0x0, 0xff},
        {0x0, 0x0, 0x0, 0xff},
        {0x0, 0x0, 0xff, 0xff},
        {},
        {0x0, 0x0, 0x00ff00, 0x0},
        {0x0, 0x00ff00, 0x00ff00, 0x00ff00},
        {0x0, 0x00ff00, 0x0, 0x00ff00},
        {},
        {0x0, 0xffff00, 0x0, 0xffff00},
        {0x0, 0x0, 0xffff00, 0x0},
        {0x0, 0x0, 0xffff00, 0x0}
    };

    std::memcpy(drawBuffer, menuBuffer, GRID_H * GRID_W * sizeof(CRGB));

    if ((inputBitmap & KEY_A) || (inputBitmap & KEY_C)) {
        gameState = GameState::WAIT_ON_INPUT;
    }
}

void waitOnInput() {
    uint8_t layer = playerInfos[activePlayerIdx].activeLayer;

    bool keyPressed = (inputBitmap & KEY_A) || (inputBitmap & KEY_C);
    if (!keyPressed) {
        // Mod for wrapping the head around
        stackWiggleHeadPos = ++stackWiggleHeadPos % GRID_W;
    }

    // Clear all LEDS on layer
    for (int i = 0; i < GRID_W; i++) {
        drawBuffer[layer][i] = UNOCCUPIED_COL;
    }

    // Display the currently placing stack
    for (int i = 0; i < STACK_WIDTH; i++) {
        drawBuffer[layer][(stackWiggleHeadPos + i) % GRID_W] = layerToColor[layer];
    }

    // If button clicked then transition to next state
    if ((inputBitmap & KEY_A) || (inputBitmap & KEY_C)) {
        M5.Lcd.printf("A");

        Serial.printf("layer: %d\n", layer);
        // Check for win
        if (layer == 255) {
            Serial.println("WIN");
            gameState = GameState::WIN;
            return;
        }

        // Check for losing
        if (layer != GRID_H - 1) { // Don't fail on first click
            int support = UNOCCUPIED; // How many tiles that are occupied below the placing stack
            for (int i = 0; i < STACK_WIDTH; i++) {
                support += playerInfos[activePlayerIdx]
                    .occupiedTiles[layer + 1][(stackWiggleHeadPos + i) % GRID_W];
            }

            // No tiles under placed stack - player looses
            if (support == UNOCCUPIED) {
                Serial.printf("LOSE!\n");
                M5.Lcd.printf("LOSE!\n");

                playerInfos[activePlayerIdx].lost = true;

                // If all players haeve lost then go to lose screen
                bool someoneAlive = false;
                for (int i = 0; i < PLAYER_COUNT; i++) {
                    someoneAlive |= !playerInfos[i].lost;
                }

                if (!someoneAlive) {
                    gameState = GameState::GAMEOVER;
                }
                else {
                    gameState = GameState::LOSE;
                }
                return;
            }
        }

        // Write the stack to the occupied tile grid
        for (int i = 0; i < STACK_WIDTH; i++) {
            playerInfos[activePlayerIdx]
                .nextOccupiedTiles[layer][(stackWiggleHeadPos + i) % GRID_W] = 1;
        }

        gameState = GameState::TILES_FALLING;
        playerInfos[activePlayerIdx].activeLayer--;
        playerInfos[activePlayerIdx].simSpeed *= SPEED_FACTOR;
    }
}

void tilesFalling() {
    // No tiles can be placed above this layer
    int topLayer = playerInfos[activePlayerIdx].activeLayer - 1;
    int movedTilesThisTick = 0;

    // Traverse from top layer to bottom (not including) of grid
    for (int row = topLayer; row + 1 < GRID_H; row++) {
        for (int col = 0; col < GRID_W; col++) {
            // The state of this tile at (row, coll)
            uint8_t tile = playerInfos[activePlayerIdx].occupiedTiles[row][col];
            if (tile == UNOCCUPIED) continue;

            // Move the tile down if nothing below it - simulate it falling
            uint8_t tileBelow = playerInfos[activePlayerIdx].occupiedTiles[row + 1][col];

            // No tile below - simulate falling
            if (tileBelow == UNOCCUPIED) {
                movedTilesThisTick++;
                playerInfos[activePlayerIdx].nextOccupiedTiles[row][col] = UNOCCUPIED;
                playerInfos[activePlayerIdx].nextOccupiedTiles[row + 1][col] = FALLING_TILE;
            }
            // The tile below is occupied - destroy the falling tile
            else if (tile == FALLING_TILE){
                playerInfos[activePlayerIdx].nextOccupiedTiles[row][col] = UNOCCUPIED;
            }
        }
    }

    // Clear falling tiles from bottom
    for (int col = 0; col < GRID_W; col++) {
        if (playerInfos[activePlayerIdx].occupiedTiles[GRID_H - 1][col] == FALLING_TILE)
            playerInfos[activePlayerIdx].nextOccupiedTiles[GRID_H - 1][col] = UNOCCUPIED;
    }

    // No more tiles to move
    if (movedTilesThisTick == 0) {
        stackWiggleHeadPos++; // Avoid the stack to spawn the same postion as last
        nextAlivePlayer();
        gameState = GameState::WAIT_ON_INPUT;
    }
}

void loosing() {
    for (int row = 0; row < GRID_H; row++) {
        for (int col = 0; col < GRID_W; col++) {
            drawBuffer[row][col] = LOSE_COL;
        }
    }

    showDrawBuffer();
    vTaskDelay(DEATH_ANIMATION_DELAY);
    gameState = GameState::TILES_FALLING;
}

void gameOver() {
    for (int row = 0; row < GRID_H; row++) {
        for (int col = 0; col < GRID_W; col++) {
            drawBuffer[row][col] = LOSE_COL;
        }
    }

    showDrawBuffer();
    vTaskDelay(DEATH_ANIMATION_DELAY);
    reset();

    gameState = GameState::MENU;
}

void winning() {
    for (int row = 0; row < GRID_H; row++) {
        for (int col = 0; col < GRID_W; col++) {
            drawBuffer[row][col] = layerToColor[row];
        }
    }

    showDrawBuffer();
    vTaskDelay(WIN_ANIMATION_DELAY);
    reset();

    gameState = GameState::MENU;
}

// Sets the activePlayerIdx to the next player that is alive
void nextAlivePlayer() {
    for (int i = 1; i < PLAYER_COUNT; i++) {
        int idx = (i + activePlayerIdx) % PLAYER_COUNT;
        if (!playerInfos[idx].lost) {
            activePlayerIdx = idx;
            return;
        }
    }
}

// Reset values to run game again
void reset() {
    for (int i = 0; i < PLAYER_COUNT; i++) {
        std::memset(playerInfos[i].occupiedTiles, 0, GRID_H * GRID_W);
        std::memset(playerInfos[i].nextOccupiedTiles, 0, GRID_H * GRID_W);
        playerInfos[i].activeLayer = GRID_H - 1;
        playerInfos[i].simSpeed = DELTA_TIME_DEFAULT;
        playerInfos[i].lost = false;
    }

    activePlayerIdx = 0;
    stackWiggleHeadPos = 0;
}

void showDrawBuffer() {
    for (int row = 0; row < GRID_H; row++) {
        for (int col = 0; col < GRID_W; col++) {
            uint8_t idx = grid[row][col];
            leds[idx] = drawBuffer[row][col];
        }
    }
    FastLED.show();
}
