#include <M5StickCPlus.h>
#include "FastLED.h"
#include <vector>
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

enum GameState {
    MENU,
    PLAYING
};

// Store LEDs in array
CRGB leds[NUM_LEDS];
uint8_t gHue                              = 0;
static TaskHandle_t FastLEDshowTaskHandle = 0;

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
GameState gameState = GameState::PLAYING;

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
    M5.Lcd.println("Display");
    M5.Lcd.setCursor(30, 55);

    // Initalize LED array
    FastLED.addLeds<WS2811, Neopixel_PIN, GRB>(leds, NUM_LEDS)
        .setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(10);
    // xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2,
    //                         NULL, 0);

    // init draw buffer to all red
    for (int i = 0; i < GRID_H; i++) {
        for (int j = 0; j < GRID_W; j++) {
            // drawBuffer[i][j] = CRGB::White;
        }
    }
    showDrawBuffer();
}

void loop() {
    switch (gameState) {
        case MENU:
            break;

        case PLAYING:
            playLoop();
            break;

        default:
            break;
    }
}

void playLoop() {

}

void showDrawBuffer() {
    for (int row = 0; row < GRID_H; row++) {
        for (int col = 0; col < GRID_W; col++) {
            uint8_t idx = grid[row][col];
            M5.Lcd.printf("%d, %d: idx: %d, color: %d", row, col, idx, drawBuffer[row][col]);
            leds[idx] = drawBuffer[row][col];
        }
    }
    FastLED.show();
}

void FastLEDshowTask(void *pvParameters) {
    for (;;) {
        fill_rainbow(leds, NUM_LEDS, gHue, 7);  // rainbow effect
        FastLED.show();  // must be executed for neopixel becoming effective
        EVERY_N_MILLISECONDS(20) {
            gHue++;
        }
    }
}

