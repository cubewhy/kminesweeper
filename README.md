# KMinesweeper

Play minesweeper in Linux kernel!

## How to play

Use `cat /dev/minesweeper` to view the board.

Use `echo "<command>" > /dev/minesweeper` to play

### Available Commands

| Command     | Desc                       |
| ----------- | -------------------------- |
| `r`         | Reset the game             |
| `c <x> <y>` | Click a grid cell          |
| `f <x> <y>` | Mark a grid cell as a mine |

## How to Install

- Clone the repository
- Make sure you have the `linux-dev` (or similiar) packages installed
- Run `make`
- Run `sudo insmod minesweeper.ko`

## License

This project is licensed under GPL-2.0.
