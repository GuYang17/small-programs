#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#define PROGRAM_NAME "gytyping"
#define AUTHOR "GuYang17"
#define VERSION "1.0.0"
#define MAX_FALLING 50
#define HELP_TEXT "Usage: gytyping [OPTIONS]\n\
\n\
Options:\n\
  -d, --difficulty LEVEL   Set difficulty level (1, 2, or 3)\n\
                           1: Slow, lowercase only\n\
                           2: Medium, lowercase only\n\
                           3: Fast, may spawn uppercase letters\n\
  -h, --help               Show this help message\n\
  -v, --version            Show version information\n"

/* Structure representing a falling letter */
typedef struct {
    int x;          /* Column position */
    int y;          /* Row position */
    char ch;        /* Character (lowercase or uppercase) */
    int active;     /* Whether the letter is currently falling */
} FallingLetter;

FallingLetter letters[MAX_FALLING];
int score = 0;
char first_target = 'a';    /* Highest priority target (closest to bottom) */
char second_target = 'a';   /* Second priority target */
int height, width;          /* Terminal dimensions */
int frame_count = 0;
int difficulty = 2;         /* Default difficulty level */
int base_speed;             /* Frame delay in milliseconds */
int spawn_delay;            /* Frames between spawns */

/* Parse command line arguments */
void parse_args(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("%s", HELP_TEXT);
            exit(0);
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("%s version %s\n", PROGRAM_NAME, VERSION);
            printf("Author: %s\n", AUTHOR);
            exit(0);
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--difficulty") == 0) {
            if (i + 1 < argc) {
                int d = atoi(argv[i + 1]);
                if (d >= 1 && d <= 3) {
                    difficulty = d;
                } else {
                    fprintf(stderr, "Error: Difficulty must be 1, 2, or 3\n");
                    exit(1);
                }
                i++;
            } else {
                fprintf(stderr, "Error: --difficulty requires an argument\n");
                exit(1);
            }
        }
        else {
            fprintf(stderr, "Error: Unknown option %s\n", argv[i]);
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            exit(1);
        }
    }
}

/* Configure game parameters based on difficulty level */
void set_difficulty() {
    switch(difficulty) {
        case 1:
            base_speed = 200;   /* Slowest falling speed */
            spawn_delay = 40;   /* Longest spawn interval */
            break;
        case 2:
            base_speed = 150;
            spawn_delay = 30;
            break;
        case 3:
            base_speed = 90;    /* Fastest falling speed */
            spawn_delay = 15;    /* Shortest spawn interval */
            break;
    }
}

/* Generate a random character based on difficulty */
char get_random_char() {
    if (difficulty == 3 && (rand() % 100) < 30) {
        return 'A' + rand() % 26;   /* Uppercase letter (30% chance on difficulty 3) */
    } else {
        return 'a' + rand() % 26;   /* Lowercase letter */
    }
}

/* Spawn a new letter at the top, only if no letter exists at row 1 */
void spawn_letter() {
    int i;
    
    /* Count letters already at the top row */
    int top_count = 0;
    for (i = 0; i < MAX_FALLING; i++) {
        if (letters[i].active && letters[i].y == 1) {
            top_count++;
        }
    }
    
    /* Prevent multiple letters on the same row */
    if (top_count >= 1) {
        return;
    }
    
    /* Find an inactive slot and create a new falling letter */
    for (i = 0; i < MAX_FALLING; i++) {
        if (!letters[i].active) {
            letters[i].x = rand() % (width - 2) + 1;
            letters[i].y = 1;
            letters[i].ch = get_random_char();
            letters[i].active = 1;
            break;
        }
    }
}

/* Draw all active letters with color coding */
void draw_letters(WINDOW *win) {
    int i;
    int first_found = 0;
    int second_found = 0;
    
    for (i = 0; i < MAX_FALLING; i++) {
        if (letters[i].active) {
            char display_char = letters[i].ch;
            
            /* Highest priority target - Red + Bold */
            if (!first_found && tolower(letters[i].ch) == tolower(first_target)) {
                wattron(win, COLOR_PAIR(1));
                wattron(win, A_BOLD);
                mvwaddch(win, letters[i].y, letters[i].x, display_char);
                wattroff(win, A_BOLD);
                wattroff(win, COLOR_PAIR(1));
                first_found = 1;
            }
            /* Second priority target - Yellow */
            else if (!second_found && tolower(letters[i].ch) == tolower(second_target) && tolower(letters[i].ch) != tolower(first_target)) {
                wattron(win, COLOR_PAIR(2));
                mvwaddch(win, letters[i].y, letters[i].x, display_char);
                wattroff(win, COLOR_PAIR(2));
                second_found = 1;
            }
            /* Other letters - Green */
            else {
                wattron(win, COLOR_PAIR(3));
                mvwaddch(win, letters[i].y, letters[i].x, display_char);
                wattroff(win, COLOR_PAIR(3));
            }
        }
    }
}

/* Move all active letters down one row, return 1 if any reached bottom */
int update_positions() {
    int i;
    int game_over = 0;
    for (i = 0; i < MAX_FALLING; i++) {
        if (letters[i].active) {
            letters[i].y++;
            if (letters[i].y >= height - 1) {
                game_over = 1;   /* Letter hit the bottom line */
            }
        }
    }
    return game_over;
}

/* Remove a letter matching the given character (case-insensitive) */
int remove_matching_letter(char c) {
    int i;
    for (i = 0; i < MAX_FALLING; i++) {
        if (letters[i].active && tolower(letters[i].ch) == tolower(c)) {
            letters[i].active = 0;
            return 1;
        }
    }
    return 0;
}

/* Update target priorities based on vertical position */
void update_targets() {
    int i;
    int highest_y = -1;        /* Largest Y (closest to bottom) */
    int second_highest_y = -1;
    char highest_ch = 'a';
    char second_highest_ch = 'a';
    
    /* Find the two letters with the largest Y coordinates */
    for (i = 0; i < MAX_FALLING; i++) {
        if (letters[i].active) {
            if (letters[i].y > highest_y) {
                second_highest_y = highest_y;
                second_highest_ch = highest_ch;
                highest_y = letters[i].y;
                highest_ch = letters[i].ch;
            }
            else if (letters[i].y > second_highest_y && letters[i].y != highest_y) {
                second_highest_y = letters[i].y;
                second_highest_ch = letters[i].ch;
            }
        }
    }
    
    if (highest_y != -1) {
        first_target = highest_ch;
    }
    if (second_highest_y != -1) {
        second_target = second_highest_ch;
    } else {
        second_target = first_target;
    }
}

/* Draw the status bar at the bottom of the screen */
void draw_status_bar() {
    char status_text[100];
    sprintf(status_text, " Score: %d | Press Esc to quit ", score);
    
    attron(COLOR_PAIR(4));  /* White text on black background */
    attron(A_BOLD);
    
    /* Clear the entire status line */
    for (int i = 0; i < width; i++) {
        mvaddch(height, i, ' ');
    }
    
    mvprintw(height, 0, "%s", status_text);
    
    attroff(A_BOLD);
    attroff(COLOR_PAIR(4));
    refresh();
}

int main(int argc, char *argv[]) {
    int ch;
    int game_over = 0;
    
    /* Parse command line arguments and configure difficulty */
    parse_args(argc, argv);
    set_difficulty();
    
    /* Initialize random number generator */
    srand(time(NULL));
    
    /* Initialize ncurses */
    initscr();
    cbreak();           /* Disable line buffering */
    noecho();           /* Don't echo typed characters */
    keypad(stdscr, TRUE);   /* Enable special keys */
    nodelay(stdscr, TRUE);  /* Non-blocking input */
    curs_set(0);        /* Hide cursor */
    
    /* Initialize color pairs if supported */
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);     /* First target */
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);  /* Second target */
        init_pair(3, COLOR_GREEN, COLOR_BLACK);   /* Other letters */
        init_pair(4, COLOR_BLACK, COLOR_WHITE);   /* Status bar */
    }
    
    /* Get terminal dimensions */
    getmaxyx(stdscr, height, width);
    height = height - 1;    /* Reserve one line for status bar */
    
    /* Create game window with border */
    WINDOW *game_win = newwin(height, width, 0, 0);
    box(game_win, 0, 0);
    wrefresh(game_win);
    
    draw_status_bar();
    
    /* Spawn initial letters */
    for (int i = 0; i < 3; i++) {
        spawn_letter();
    }
    update_targets();
    
    /* Main game loop */
    while (!game_over) {
        /* Handle keyboard input */
        ch = getch();
        if (ch != ERR) {
            if (ch == 27) {     /* ESC key - exit game */
                break;
            }
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
                if (tolower(ch) == tolower(first_target)) {
                    /* Correct letter */
                    if (remove_matching_letter(ch)) {
                        score += 5;
                        draw_status_bar();
                        spawn_letter();
                        update_targets();
                    }
                } else {
                    /* Wrong letter - penalty */
                    score -= 1;
                    draw_status_bar();
                    flash();     /* Flash screen as feedback */
                }
            }
        }
        
        /* Update falling positions and check game over */
        game_over = update_positions();
        
        /* Redraw game window */
        werase(game_win);
        box(game_win, 0, 0);
        draw_letters(game_win);
        wrefresh(game_win);
        
        /* Control game speed */
        napms(base_speed);
        frame_count++;
        
        /* Spawn new letters periodically */
        if (frame_count % spawn_delay == 0 && frame_count > 0) {
            spawn_letter();
            update_targets();
        }
    }
    
    /* Cleanup */
    delwin(game_win);
    endwin();
    
    /* Display final score after exiting TUI */
    printf("Game over! Your final score: %d\n", score);
    
    return 0;
}
