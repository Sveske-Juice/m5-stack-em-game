# m5-stack-em-game
Stacker arcade game designed for LED strips positioned in a grid
 using an M5StickC Plus micro controller

# Install Instructions

1. Setup Arduino IDE development with M5StickC Plus: https://docs.m5stack.com/en/quick_start/m5stickc_plus/arduino
2. clone repo
3. Open the code with arduino IDE (main.ino)
4. (optional) modify the code or settings

## To flash:
1. In arduino IDE under Tools->Board Select "M5Stick-C-Plus"
2. Connect the M5 to the PC's USB port
3. In arduino IDE under Tools->Port Select the USB port connected to the M5
4. Click upload button which will compile and flash the M5

## Custom button
1. Connect one wire to M5's ground (GND)
2. Connect other wire to M5's G26 pin (specified by `KEY_C_PIN` in code)

See https://docs.m5stack.com/en/api/core/gpio for M5 pin name to arduino pin code reference.

# Compiler settings
`PLAYER_COUNT`: the amount of players that plays the game (default is 2)
`STACK_WIDTH`: the width of the placing layer (default is 1)
`SPEED_FACTOR`: each layer progressed this factor will be multiplied,
which affects the speed of the game (the delay between game ticks)

See comments for other options

# Usage

Press the button to place the stack
