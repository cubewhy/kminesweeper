# KMinesweeper

Play minesweeper in Linux kernel!

```text
$ cat /dev/minesweeper

=== KERNEL MINESWEEPER ===
   0 1 2 3 4 5 6 7 8
0  1 . . . . . . . .
1  . . . . . . . . .
2  1 1 2 . . . . . .
3      3 . . . . 1 1
4    1 3 . 2 . . 1
5    1 . . . . . 1
6    1 1 1 1 . 2 1
7          1 1 1
8

[STATUS]: Playing (Mines: 10)
Commands: 'c X Y' (click), 'f X Y' (flag), 'r' (reset)
```

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
