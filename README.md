SDL2 Tetris
===========

Incomplete Tetris clone that needs to be cleaned up

How to Build
------------

1. Make sure you have dependencies `SDL2` and `SDL2_ttf`
2. `git clone https://github.com/aderosier/sdl2-tetris.git sdl2-tetris`
3. `cd sdl2-tetris`
4. `make`

Controls
--------

- Arrow left/right - Move left/right
- Arrow down - Soft Drop
- Space - Hard Drop
- Z - Rotate left (counter clockwise)
- X, Arrow up - Rotate right (clockwise)
- Shift - Hold
- Enter - Pause

TODO
----

Game Mechanics:
- [x] Playfield size 10x22, top 2 rows not visible
- [x] Tetromino colors match guidelines
- [x] Tetrominos start at middle-left, top row (-2)
- [ ] The tetrominos spawn horizontally and with their flat side pointed down
- [x] Super Rotation System http://tetris.wikia.com/wiki/SRS
- [x] Wall kicks/floor kicks
- [x] Lock delay http://tetris.wikia.com/wiki/Infinity
- [x] "Random Bag" system http://tetris.wikia.com/wiki/Random_Generator
- [x] Hold piece
- [x] Ghost piece
- [x] Designated soft drop speed
- [x] Player may only level up by clearing lines or performing T-Spin
- [x] The player tops out when a piece is spawned overlapping at least one block
- [x] Display of next-coming tetrominos
- [x] T-spins
- [ ] Rewarding of back to back chains
- [x] Delayed Auto Shift http://tetris.wikia.com/wiki/DAS

Code Quality:
- [ ] More easily understandable block layout for tetrominos
- [ ] Consistent naming convention
- [x] Split generic stuff into modules
- [ ] Explain what each function does
