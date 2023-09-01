from m5stack import *
from m5ui import *
from uiflow import *
import unit

GRID_WIDTH = 4
GRID_HEIGHT = 15
setScreenColor(0xff0000)
neopixel_0 = unit.get(unit.NEOPIXEL, unit.PORTA, 120)

BUN_COL = 0x28dee8
KETCHUP_COL = 0xff0000
MUSTARD_COL = 0xffea00
BEEF_COL = 0x1d1405
CHEESE_COL = 0xe5a40d
BURGER_SAUCE_COL = 0x534822
BACON_COL = 0xba1f1f
SALAD_COL = 0x12ba17
OLIVE_COL = 0x6f1d61
MAYO_COL = 0xeadf69

grid = None
row = None
i = None

drawBuffer = None

grid = [[103, 63, 46, 1], [None, None, None, None], [None, None, None, None], [None, None, None, None], [None, None, None, None], [None, None, None, None], [None, None, None, None], [None, None, None, None], [None, None, None, None], [None, None, None, None], [None, None, None, None], [None, None, None, None], [None, None, None, None], [None, None, None, None], [None, None, None, None]]
for i in range(1, GRID_HEIGHT):
  grid[i][0] = grid[i-1][0] - 1
  grid[i][2] = grid[i-1][2] - 1
  grid[i][1] = grid[i-1][1] + 1
  grid[i][3] = grid[i-1][3] + 1

drawBuffer = [[0xffffff]*GRID_WIDTH]*GRID_HEIGHT

drawBuffer[GRID_HEIGHT-1] = [BUN_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-2] = [KETCHUP_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-3] = [MUSTARD_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-4] = [BEEF_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-5] = [CHEESE_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-6] = [BUN_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-7] = [BURGER_SAUCE_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-8] = [BEEF_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-9] = [BACON_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-10] = [SALAD_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-11] = [OLIVE_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-12] = [KETCHUP_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-13] = [MAYO_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-14] = [BEEF_COL]*GRID_WIDTH
drawBuffer[GRID_HEIGHT-15] = [BUN_COL]*GRID_WIDTH


for row in range(GRID_HEIGHT):
  for col in range(GRID_WIDTH):
    neopixel_0.setColor(grid[row][col], drawBuffer[row][col])

