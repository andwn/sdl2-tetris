SDL2 Tetris
===========

Incomplete Tetris clone that needs to be cleaned up

How to Build
------------

1. Get dependencies `SDL2` and `SDL2_ttf`
2. `git clone https://github.com/aderosier/sdl2-tetris.git sdl2-tetris`
3. `cd sdl2-tetris`
4. `make`

Controls
--------

- Arrow left/right - Move left/right
- Arrow up, Z, X - Rotate
- Arrow down - Speed up
- Space - Drop
- Shift - Hold
- Enter - Pause

TODO
----

Guideline checklist http://tetris.wikia.com/wiki/Tetris_Guideline
>[DONE] Playfield size
>[DONE] Tetromino colors
>[DONE] Tetromino start locations
>[TODO] The tetrominoes spawn horizontally and with their flat side pointed down
>[TODO] Super Rotation System http://tetris.wikia.com/wiki/SRS
>[TODO] Lock delay http://tetris.wikia.com/wiki/Infinity
>[TODO] Standard mappings for computer keyboards
>[TODO] http://tetris.wikia.com/wiki/Random_Generator
>[DONE] Hold piece
>[DONE] Ghost piece
>[DONE] Designated soft drop speed
>[DONE] Player may only level up by clearing lines or performing T-Spin
>[TODO] The player tops out when a piece is spawned overlapping at least one block
>
>[DONE] Display of next-coming tetrominoes
>[TODO] T-spins
>[TODO] Rewarding of back to back chains
>[TODO] Delayed Auto Shift http://tetris.wikia.com/wiki/DAS

Things I'm not bothering with
- "Tetriminos" not "pieces", letter names not "square" nor "stick", etc
- The game must use a variant of Roger Dean's Tetris logo
- Game must include a song called Korobeiniki. (Guideline 2005~)
- Game should include the songs Katjusha, or Kalinka
