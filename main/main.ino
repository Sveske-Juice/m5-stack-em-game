#include <M5StickCPlus.h>
#include "FastLED.h"
#include <vector>
#include <unordered_map>
#include <stdint.h>

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
#define OLIVE_COL 0x6f1d61
#define MAYO_COL 0xeadf69
#define UNOCCUPIED_COL 0x00

// Settings
#define DELTA_TIME_MILLIS 250

#define PLAYER_COUNT 2
#define STACK_WIDTH 2

std::unordered_map<uint8_t, int> layerToColor = {
    {14, BUN_COL},
    {13, KETCHUP_COL},
    {12, MUSTARD_COL},
    {11, BEEF_COL},
    {10, CHEESE_COL},
    {9, BURGER_SAUCE_COL},
    {8, BACON_COL},
    {7, SALAD_COL},
    {6, OLIVE_COL},
    {5, MAYO_COL},
    {4, MAYO_COL},
    {3, MAYO_COL},
    {2, MAYO_COL},
    {1, MAYO_COL},
    {0, MAYO_COL}
};

enum GameState {
    // In this state while in menu
    MENU,

    // In this state when the game is waiting for the user to place the layer
    // I.e the layer should shift left to right in this state
    WAIT_ON_INPUT,
};

struct PlayerData {
    // Stores all the tiles the player has a block placed.
    // 1: a block is placed at the position
    // 0: No block is placed at that position
    // TODO: switch to use a bitmap since only 1bit of info is needed per point
    uint8_t occupiedTiles[GRID_H][GRID_W];

    // The layer this player has reached
    uint8_t activeLayer = GRID_H - 1;
};


PlayerData playerInfos[PLAYER_COUNT];

int activePlayerIdx = 0;

// The index position of the head on the active layer
// Used to keep track on the currently placing stack
// in the WAIT_ON_INPUT state
int stackWiggleHeadPos = 0;

CRGB leds[NUM_LEDS];
GameState gameState = GameState::WAIT_ON_INPUT;

// Bitmap representing if a button have been clicked within the last update tick
// idx 001: Btn A
// idx 010: Btn B
// idx 100: Btn C (custom button)
int inputBitmap = 0;

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
    // M5.Lcd.fillScreen(
    //         BLACK);
    // M5.Lcd.setCursor(0,0);
    // M5.Lcd.printf("ch: %d\n", M5.BtnA.lastChange());
    if (M5.BtnA.wasReleased())
        inputBitmap |= 1;

    if (M5.BtnB.wasReleased())
        inputBitmap |= 1 << 1;

    // TODO: check for custom btn
}

void gameLoop(void* params) {
    for (;;) {
        switch (gameState) {
            case MENU:
                break;

            case WAIT_ON_INPUT:
                waitOnInput();
                break;

            default:
                break;
        }
        showDrawBuffer();

        // Reset input from this tick - accumulate input events until next tick
        inputBitmap = 0;

        // FIXME: subtract time took to tick game
        vTaskDelay(DELTA_TIME_MILLIS / portTICK_PERIOD_MS);
    }
}

void setup() {
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

    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(20, 2);
    M5.Lcd.println("Stack' em'");
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(15, 35);

    // Initalize LED array
    FastLED.addLeds<WS2811, Neopixel_PIN, GRB>(leds, NUM_LEDS)
        .setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(10);
    // xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2,
    //                         NULL, 0);

    xTaskCreatePinnedToCore(gameLoop, "Game Loop", 4096, NULL, 1, NULL, 0);
    // init draw buffer
    for (int i = 0; i < GRID_H; i++) {
        for (int j = 0; j < GRID_W; j++) {
            drawBuffer[i][j] = UNOCCUPIED_COL;
        }
    }
}

int cyclesSinceUpdate = 0;
void loop() {
    // Update internal m5 stuff like input
    M5.update();
    pollInput();
}

void waitOnInput() {
    if (inputBitmap & 1) {
        M5.Lcd.printf("A");
    }

    uint8_t layer = playerInfos[activePlayerIdx].activeLayer;

    // Mod for wrapping the head around
    stackWiggleHeadPos = ++stackWiggleHeadPos % GRID_W;
    // M5.Lcd.printf("%d, ", stackWiggleHeadPos);

    // Clear all LEDS on layer
    for (int i = 0; i < GRID_W; i++) {
        drawBuffer[layer][i] = UNOCCUPIED_COL;
    }

    // Display the currently placing stack
    for (int i = 0; i < STACK_WIDTH; i++) {
        // if (i > GRID_W) break;
        drawBuffer[layer][(stackWiggleHeadPos + i) % GRID_W] = layerToColor[layer];
    }
}

void showDrawBuffer() {
    for (int row = 0; row < GRID_H; row++) {
        for (int col = 0; col < GRID_W; col++) {
            uint8_t idx = grid[row][col];
            // M5.Lcd.printf("%d, %d: idx: %d, color: %d", row, col, idx, drawBuffer[row][col]);
            leds[idx] = drawBuffer[row][col];
        }
    }
    FastLED.show();
}
