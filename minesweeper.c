#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/mutex.h>
#include <linux/string.h>

#define BOARD_W 9
#define BOARD_H 9
#define TOTAL_MINES 10
#define BUF_SIZE 2048

enum cell_visibility {
    CELL_HIDDEN = 0,
    CELL_REVEALED,
    CELL_FLAGGED
};

enum game_status {
    GAME_ONGOING = 0,
    GAME_WON,
    GAME_LOST
};

static DEFINE_MUTEX(ms_lock);

static bool mines[BOARD_H][BOARD_W];
static int adjacent[BOARD_H][BOARD_W];
static u8 state[BOARD_H][BOARD_W];
static int status = GAME_ONGOING;

static void init_game(void)
{
    int placed = 0;
    int x, y, dx, dy;

    memset(mines, 0, sizeof(mines));
    memset(adjacent, 0, sizeof(adjacent));
    memset(state, CELL_HIDDEN, sizeof(state));
    status = GAME_ONGOING;

    while (placed < TOTAL_MINES) {
        u32 r;
        get_random_bytes(&r, sizeof(r));
        x = (r & 0xFFFF) % BOARD_W;
        y = ((r >> 16) & 0xFFFF) % BOARD_H;

        if (!mines[y][x]) {
            mines[y][x] = true;
            placed++;
        }
    }

    for (y = 0; y < BOARD_H; y++) {
        for (x = 0; x < BOARD_W; x++) {
            if (mines[y][x])
                continue;
            for (dy = -1; dy <= 1; dy++) {
                for (dx = -1; dx <= 1; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;
                    if (nx >= 0 && nx < BOARD_W && ny >= 0 && ny < BOARD_H) {
                        if (mines[ny][nx])
                            adjacent[y][x]++;
                    }
                }
            }
        }
    }
}

static void check_win(void)
{
    int x, y;
    for (y = 0; y < BOARD_H; y++) {
        for (x = 0; x < BOARD_W; x++) {
            if (!mines[y][x] && state[y][x] != CELL_REVEALED)
                return;
        }
    }
    status = GAME_WON;
}

static void reveal_blank_cells(int start_x, int start_y)
{
    int stack_x[BOARD_W * BOARD_H];
    int stack_y[BOARD_W * BOARD_H];
    int top = 0;

    stack_x[top] = start_x;
    stack_y[top] = start_y;
    top++;

    while (top > 0) {
        int x, y, dx, dy;
        top--;
        x = stack_x[top];
        y = stack_y[top];

        if (adjacent[y][x] != 0)
            continue;

        for (dy = -1; dy <= 1; dy++) {
            for (dx = -1; dx <= 1; dx++) {
                int nx = x + dx;
                int ny = y + dy;
                if (nx >= 0 && nx < BOARD_W && ny >= 0 && ny < BOARD_H) {
                    if (state[ny][nx] == CELL_HIDDEN) {
                        state[ny][nx] = CELL_REVEALED;
                        if (adjacent[ny][nx] == 0) {
                            stack_x[top] = nx;
                            stack_y[top] = ny;
                            top++;
                        }
                    }
                }
            }
        }
    }
}

static void action_click(int x, int y)
{
    if (x < 0 || x >= BOARD_W || y < 0 || y >= BOARD_H)
        return;
    if (status != GAME_ONGOING || state[y][x] != CELL_HIDDEN)
        return;

    if (mines[y][x]) {
        state[y][x] = CELL_REVEALED;
        status = GAME_LOST;
        return;
    }

    state[y][x] = CELL_REVEALED;
    if (adjacent[y][x] == 0)
        reveal_blank_cells(x, y);

    check_win();
}

static void action_flag(int x, int y)
{
    if (x < 0 || x >= BOARD_W || y < 0 || y >= BOARD_H)
        return;
    if (status != GAME_ONGOING)
        return;

    if (state[y][x] == CELL_HIDDEN)
        state[y][x] = CELL_FLAGGED;
    else if (state[y][x] == CELL_FLAGGED)
        state[y][x] = CELL_HIDDEN;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char *kbuf;
    int len = 0;
    int x, y;
    ssize_t ret;

    kbuf = kzalloc(BUF_SIZE, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    guard(mutex)(&ms_lock);

    len += scnprintf(kbuf + len, BUF_SIZE - len, "\n=== KERNEL MINESWEEPER ===\n  ");
    for (x = 0; x < BOARD_W; x++)
        len += scnprintf(kbuf + len, BUF_SIZE - len, " %d", x);
    len += scnprintf(kbuf + len, BUF_SIZE - len, "\n");

    for (y = 0; y < BOARD_H; y++) {
        len += scnprintf(kbuf + len, BUF_SIZE - len, "%d ", y);
        for (x = 0; x < BOARD_W; x++) {
            if (status == GAME_LOST && mines[y][x]) {
                len += scnprintf(kbuf + len, BUF_SIZE - len, " *");
            } else if (state[y][x] == CELL_HIDDEN) {
                len += scnprintf(kbuf + len, BUF_SIZE - len, " .");
            } else if (state[y][x] == CELL_FLAGGED) {
                len += scnprintf(kbuf + len, BUF_SIZE - len, " F");
            } else {
                if (adjacent[y][x] == 0)
                    len += scnprintf(kbuf + len, BUF_SIZE - len, "  ");
                else
                    len += scnprintf(kbuf + len, BUF_SIZE - len, " %d", adjacent[y][x]);
            }
        }
        len += scnprintf(kbuf + len, BUF_SIZE - len, "\n");
    }

    if (status == GAME_WON)
        len += scnprintf(kbuf + len, BUF_SIZE - len, "\n[STATUS]: >>> YOU WIN! <<<\n");
    else if (status == GAME_LOST)
        len += scnprintf(kbuf + len, BUF_SIZE - len, "\n[STATUS]: *** BOOM! GAME OVER ***\n");
    else
        len += scnprintf(kbuf + len, BUF_SIZE - len, "\n[STATUS]: Playing (Mines: %d)\n", TOTAL_MINES);

    len += scnprintf(kbuf + len, BUF_SIZE - len,
                     "Commands: 'c X Y' (click), 'f X Y' (flag), 'r' (reset)\n\n");

    ret = simple_read_from_buffer(buf, count, ppos, kbuf, len);
    kfree(kbuf);
    return ret;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf[64];
    char cmd;
    int x, y;
    size_t copy_len = min(count, sizeof(kbuf) - 1);

    if (copy_from_user(kbuf, buf, copy_len))
        return -EFAULT;

    kbuf[copy_len] = '\0';

    guard(mutex)(&ms_lock);

    if (strstarts(kbuf, "reset") || strstarts(kbuf, "r")) {
        init_game();
    } else if (sscanf(kbuf, "%c %d %d", &cmd, &x, &y) == 3) {
        if (cmd == 'c')
            action_click(x, y);
        else if (cmd == 'f')
            action_flag(x, y);
    }

    return count;
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .read    = dev_read,
    .write   = dev_write,
};

static struct miscdevice ms_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "minesweeper",
    .fops  = &fops,
    .mode  = 0666,
};

static int __init minesweeper_init(void)
{
    int ret;
    init_game();
    ret = misc_register(&ms_dev);
    if (ret) {
        pr_err("minesweeper: failed to register device\n");
        return ret;
    }
    pr_info("minesweeper: /dev/minesweeper loaded.\n");
    return 0;
}

static void __exit minesweeper_exit(void)
{
    misc_deregister(&ms_dev);
    pr_info("minesweeper: /dev/minesweeper unloaded.\n");
}

module_init(minesweeper_init);
module_exit(minesweeper_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mingyou Chen <qby140326@gmail.com>");
MODULE_DESCRIPTION("Play Minesweeper inside the Linux Kernel");
MODULE_VERSION("1.0");
