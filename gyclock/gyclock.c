#include <ncurses.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Program information macros
#define VERSION "1.0.0"
#define AUTHOR "GuYang17"
#define PROGRAM_NAME "gyclock"
#define DESCRIPTION "A simple terminal clock program"

// Display help information
void show_help(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  --simple        Show time only (without date and weekday)\n");
    printf("  --tux           Display Tux (Linux mascot) ascii art below the clock\n");
    printf("  -h, --help      Show this help message\n");
    printf("  -v, --version   Show version information\n");
    printf("\n");
    printf("Controls:\n");
    printf("  q, Q, Esc       Exit the program\n");
    printf("\n");
    printf("%s - %s\n", DESCRIPTION, VERSION);
}

// Display version information
void show_version() {
    printf("%s version %s\n", PROGRAM_NAME, VERSION);
    printf("Written by %s\n", AUTHOR);
    printf("\n");
    printf("Features:\n");
    printf("  - Real-time clock display\n");
    printf("  - Date and weekday display (optional)\n");
    printf("  - Tux ascii art (optional)\n");
    printf("  - Centered layout\n");
    printf("  - ncurses-based terminal UI\n");
}

// Draw Tux the Linux penguin mascot as ascii art
void draw_tux(int start_y, int start_x) {
    const char *tux[] = {
        "          .--.",
        "         |o_o |",
        "         |:_/ |",
        "        //   \\ \\",
        "       (|     | )",
        "      /'\\_   _/`\\",
        "      \\___)=(___/",
        "          U"
    };
    
    int num_lines = sizeof(tux) / sizeof(tux[0]);
    
    // Print each line of the ascii art
    for (int i = 0; i < num_lines; i++) {
        mvprintw(start_y + i, start_x, "%s", tux[i]);
    }
}

int main(int argc, char *argv[]) {
    // Command line flags
    int simple_mode = 0;  // Time-only display mode
    int tux_mode = 0;     // Show Tux ascii art
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            show_version();
            return 0;
        } else if (strcmp(argv[i], "--simple") == 0) {
            simple_mode = 1;
        } else if (strcmp(argv[i], "--tux") == 0) {
            tux_mode = 1;
        } else {
            printf("Unknown option: %s\n", argv[i]);
            printf("Try '%s --help' for more information.\n", argv[0]);
            return 1;
        }
    }
    
    // Initialize ncurses
    initscr();              // Start ncurses mode
    cbreak();               // Disable line buffering
    noecho();               // Don't echo typed characters
    curs_set(0);            // Hide the cursor
    nodelay(stdscr, TRUE);  // Non-blocking input for exit detection
    
    // Get terminal dimensions
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Position variables
    int time_y, time_x;     // Time display position
    int date_y, date_x;     // Date display position
    int second_line_len;    // Length of combined date+weekday string
    int tux_y, tux_x;       // Tux ascii art position
    
    // Calculate display positions based on mode
    if (simple_mode) {
        // Simple mode: time only, centered vertically
        if (tux_mode) {
            time_y = (max_y - 8) * 45 / 100;  // Adjust position when Tux is shown
        } else {
            time_y = max_y / 2;                // Perfect center when no Tux
        }
        time_x = (max_x - 8) / 2;              // Center horizontally (8 = "HH:MM:SS")
    } else {
        // Normal mode: time + date + weekday
        if (tux_mode) {
            time_y = (max_y - 10) * 45 / 100;  // Move up to make room for Tux
            date_y = time_y + 2;                // Date appears 2 lines below time
        } else {
            time_y = max_y / 2 - 1;             // Time slightly above center
            date_y = max_y / 2 + 1;             // Date slightly below center
        }
        time_x = (max_x - 8) / 2;              // Center time horizontally
    }
    
    // Time and date variables
    time_t rawtime;
    struct tm *timeinfo;
    char time_str[9];       // HH:MM:SS + '\0'
    char date_str[11];      // YYYY-MM-DD + '\0'
    char weekday_str[4];    // Abbreviated weekday (3 chars + '\0')
    char second_line[30];   // Combined date and weekday string
    int ch;                 // Character for keyboard input
    
    // Main display loop
    while (1) {
        // Check for exit keys (non-blocking)
        ch = getch();
        if (ch == 'q' || ch == 'Q' || ch == 27) {  // 27 = ESC key
            break;
        }
        
        // Get current system time
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        
        // Format time string
        strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
        
        // Clear screen for smooth update
        clear();
        
        if (simple_mode) {
            // Simple mode: display time only with bold attribute
            attron(A_BOLD);
            mvprintw(time_y, time_x, "%s", time_str);
            attroff(A_BOLD);
            
            // Draw Tux if enabled
            if (tux_mode) {
                int tux_lines = 8;
                tux_y = time_y + 2;                      // Position below time
                tux_x = (max_x - 20) / 2 - 2;            // Center and shift left
                if (tux_y + tux_lines < max_y) {         // Ensure it fits on screen
                    draw_tux(tux_y, tux_x);
                }
            }
        } else {
            // Normal mode: display date and weekday as well
            strftime(date_str, sizeof(date_str), "%Y-%m-%d", timeinfo);
            strftime(weekday_str, sizeof(weekday_str), "%a", timeinfo);
            
            // Combine date and weekday into one line
            snprintf(second_line, sizeof(second_line), "%s %s", date_str, weekday_str);
            second_line_len = strlen(second_line);
            date_x = (max_x - second_line_len) / 2;      // Center horizontally
            
            // Display bold time
            attron(A_BOLD);
            mvprintw(time_y, time_x, "%s", time_str);
            attroff(A_BOLD);
            
            // Display date and weekday (normal text)
            mvprintw(date_y, date_x, "%s", second_line);
            
            // Draw Tux if enabled
            if (tux_mode) {
                int tux_lines = 8;
                tux_y = date_y + 2;                      // Position below date
                tux_x = (max_x - 20) / 2 - 2;            // Center and shift left
                if (tux_y + tux_lines < max_y) {         // Ensure it fits on screen
                    draw_tux(tux_y, tux_x);
                }
            }
        }
        
        // Refresh the screen to show updates
        refresh();
        
        // Small delay to reduce CPU usage (100ms)
        napms(100);
    }
    
    // Cleanup and restore terminal
    endwin();
    return 0;
}
