#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "output/shared/graphical.h"
#include "shared.h"

int gradient_size = 64;

int greatestDivisible(int a, int b) {
    int d = a%b;
    return a-d;
}

struct colors {
    NCURSES_COLOR_T color;
    NCURSES_COLOR_T R;
    NCURSES_COLOR_T G;
    NCURSES_COLOR_T B;
};

#define COLOR_REDEFINITION -2

#define MAX_COLOR_REDEFINITION 256

const wchar_t *bar_heights[] = {L"\u2581", L"\u2582", L"\u2583", L"\u2584",
    L"\u2585", L"\u2586", L"\u2587", L"\u2588"};
int num_bar_heights = (sizeof(bar_heights) / sizeof(bar_heights[0]));

// static struct colors the_color_redefinitions[MAX_COLOR_REDEFINITION];

// general: cleanup
EXP_FUNC void wavaOutputCleanup(WAVA *wava) {
    echo();
    system("setfont  >/dev/null 2>&1");
    system("setfont /usr/share/consolefonts/Lat2-Fixed16.psf.gz  >/dev/null 2>&1");
    system("setterm -blank 10  >/dev/null 2>&1");
    /*for(int i = 0; i < gradient_size; ++i) {
      if(the_color_redefinitions[i].color) {
      init_color(the_color_redefinitions[i].color,
      the_color_redefinitions[i].R,
      the_color_redefinitions[i].G,
      the_color_redefinitions[i].B);
      }
      }
      */
    standend();
    endwin();
    system("clear");
}

static void parse_color(char *color_string, struct colors *color) {
    if (color_string[0] == '#') {
        if (!can_change_color()) {
            wavaOutputCleanup(NULL); // lol
            wavaBail("Your terminal cannot change color definitions, "
                    "please use one of the predefined colors.\n");
        }
        color->color = COLOR_REDEFINITION;
        sscanf(++color_string, "%02hx%02hx%02hx", &color->R, &color->G, &color->B);
    }
}
/*
   static void remember_color_definition(NCURSES_COLOR_T color_number) {
   int index = color_number - 1; // array starts from zero and colors - not
   if(the_color_redefinitions[index].color == 0) {
   the_color_redefinitions[index].color = color_number;
   color_content(color_number,
   &the_color_redefinitions[index].R,
   &the_color_redefinitions[index].G,
   &the_color_redefinitions[index].B);
   }
   }
   */
// ncurses use color range [0, 1000], and we - [0, 255]
#define CURSES_COLOR_COEFFICIENT(X) ((X)*1000.0 / 0xFF + 0.5)
#define COLORS_STRUCT_NORMALIZE(X)                                                                 \
    CURSES_COLOR_COEFFICIENT(X.R), CURSES_COLOR_COEFFICIENT(X.G), CURSES_COLOR_COEFFICIENT(X.B)

static NCURSES_COLOR_T change_color_definition(NCURSES_COLOR_T color_number,
        char *const color_string,
        NCURSES_COLOR_T predef_color) {
    struct colors color = {0};
    parse_color(color_string, &color);
    NCURSES_COLOR_T return_color_number = predef_color;
    if (color.color == COLOR_REDEFINITION) {
        // remember_color_definition(color_number);
        init_color(color_number, COLORS_STRUCT_NORMALIZE(color));
        return_color_number = color_number;
    }
    return return_color_number;
}

EXP_FUNC int wavaInitOutput(WAVA *wava) {
    WAVA_CONFIG *conf = &wava->conf;
    initscr();
    curs_set(0);
    timeout(0);
    noecho();
    start_color();
    use_default_colors();

    int neww, newh;
    getmaxyx(stdscr, newh, neww);
    calculate_win_geo(wava, neww, newh);
    wava->bar_space.w = wava->inner.w;
    wava->bar_space.h = wava->inner.h;
    clear();

    NCURSES_COLOR_T color_pair_number = 16;

    NCURSES_COLOR_T bg_color_number;
    bg_color_number = change_color_definition(0, conf->bcolor, conf->bgcol);

    if (!conf->gradients) {

        NCURSES_COLOR_T fg_color_number;
        fg_color_number = change_color_definition(1, conf->color, conf->col);

        init_pair(color_pair_number, fg_color_number, bg_color_number);

    } else {

        // 0 -> col1, 1-> col1<=>col2, 2 -> col2 and so on
        short unsigned int rgb[2 * conf->gradients - 1][3];
        char next_color[14];

        gradient_size = wava->outer.h;

        if (gradient_size > COLORS)
            gradient_size = COLORS - 1;

        if (gradient_size > COLOR_PAIRS)
            gradient_size = COLOR_PAIRS - 1;

        if (gradient_size > MAX_COLOR_REDEFINITION)
            gradient_size = MAX_COLOR_REDEFINITION - 1;

        for (int i = 0; i < conf->gradients; i++) {
            int col = (i + 1) * 2 - 2;
            sscanf(conf->gradient_colors[i] + 1, "%02hx%02hx%02hx", &rgb[col][0], &rgb[col][1],
                    &rgb[col][2]);
        }

        // sscanf(gradient_color_1 + 1, "%02hx%02hx%02hx", &rgb[0][0], &rgb[0][1], &rgb[0][2]);
        // sscanf(gradient_color_2 + 1, "%02hx%02hx%02hx", &rgb[1][0], &rgb[1][1], &rgb[1][2]);

        int individual_size = gradient_size / (conf->gradients - 1);

        for (int i = 0; i < conf->gradients - 1; i++) {

            int col = (i + 1) * 2 - 2;
            if (i == conf->gradients - 1)
                col = 2 * (conf->gradients - 1) - 2;

            for (int j = 0; j < individual_size; j++) {

                for (int k = 0; k < 3; k++) {
                    rgb[col + 1][k] = rgb[col][k] + (rgb[col + 2][k] - rgb[col][k]) *
                        (j / (individual_size * 0.85));
                    if (rgb[col + 1][k] > 255)
                        rgb[col][k] = 0;
                    if (j > individual_size * 0.85)
                        rgb[col + 1][k] = rgb[col + 2][k];
                }

                sprintf(next_color, "#%02x%02x%02x", rgb[col + 1][0], rgb[col + 1][1],
                        rgb[col + 1][2]);

                change_color_definition(color_pair_number, next_color, color_pair_number);
                init_pair(color_pair_number, color_pair_number, bg_color_number);
                color_pair_number++;
            }
        }

        int left = individual_size * (conf->gradients - 1);
        int col = 2 * (conf->gradients)-2;
        while (left < gradient_size) {
            sprintf(next_color, "#%02x%02x%02x", rgb[col][0], rgb[col][1], rgb[col][2]);
            change_color_definition(color_pair_number, next_color, color_pair_number);
            init_pair(color_pair_number, color_pair_number, bg_color_number);
            color_pair_number++;
            left++;
        }
        color_pair_number--;
    }

    attron(COLOR_PAIR(color_pair_number));

    if (bg_color_number != -1)
        bkgd(COLOR_PAIR(color_pair_number));

    for (int y = 0; y < wava->outer.h/8; y++) {
        for (int x = 0; x < wava->outer.h; x++) {
            mvaddch(y, x, ' ');
        }
    }
    refresh();

    return 0;
}

void change_colors(int cur_height, int tot_height) {
    tot_height /= gradient_size;
    if (tot_height < 1)
        tot_height = 1;
    cur_height /= tot_height;
    if (cur_height > gradient_size - 1)
        cur_height = gradient_size - 1;
    attron(COLOR_PAIR(cur_height + 16));
}

EXP_FUNC XG_EVENT wavaOutputHandleInput(WAVA *wava) {
    WAVA_CONFIG *conf = &wava->conf;
    char ch = getch();

    int neww,newh;
    getmaxyx(stdscr, newh, neww);
    newh*=8;
    if(neww!=wava->outer.w||newh!=wava->outer.h) {
        calculate_win_geo(wava, neww, newh);
        wava->bar_space.w = wava->inner.w;
        wava->bar_space.h = wava->inner.h;
        return WAVA_RESIZE;
    }

    switch (ch) {
        case 'a':
            conf->bs++;
            return WAVA_RESIZE;
        case 's':
            if(conf->bs > 0)
                conf->bs--;
            return WAVA_RESIZE;
        case 65: // key up
            conf->sens = conf->sens * 1.05;
            break;
        case 66: // key down
            conf->sens = conf->sens * 0.95;
            break;
        case 68: // key right
            conf->bw++;
            return WAVA_RESIZE;
        case 67: // key left
            if (conf->bw > 1)
                conf->bw--;
            return WAVA_RESIZE;
        case 'r': // reload config
            return WAVA_RELOAD;
        case 'f': // change forground color
            if (conf->col < 7)
                conf->col++;
            else
                conf->col = 0;
            return WAVA_RESIZE;
        case 'b': // change backround color
            if (conf->bgcol < 7)
                conf->bgcol++;
            else
                conf->bgcol = 0;
            return WAVA_REDRAW;
        case 'q':
            return WAVA_QUIT;
    }

    return WAVA_IGNORE;
}

EXP_FUNC void wavaOutputClear(WAVA *wava) {
    system("clear");
    clear();
}

EXP_FUNC void wavaOutputApply(WAVA *wava) {
    wavaOutputClear(wava);
}

#define TERMINAL_RESIZED -1

EXP_FUNC int wavaOutputDraw(WAVA *wava) {
    WAVA_CONFIG *conf = &wava->conf;

    int height = wava->outer.h/8-1;

    for(int i=0; i<wava->bars; i++) {
        int diff=wava->f[i]-wava->fl[i];
        if(diff==0) continue;

        int xoffset = wava->rest+(conf->bw+conf->bs)*i;
        if(diff>0) {
            for(int k=greatestDivisible(wava->fl[i], 8); k<wava->f[i]; k+=8) {
                //change_colors(k, height);
                int kdiff=wava->f[i]-k; if(kdiff > 8) kdiff = 8;
                for(int j=0; j<conf->bw; j++) {
                    mvaddch(height-k/8, xoffset+j, 0x40 + kdiff);
                }
            }
        } else {
            for(int k=greatestDivisible(wava->f[i], 8); k<wava->fl[i]; k+=8) {
                int kdiff=wava->f[i]-k;
                //change_colors(k, height);
                for(int j=0; j<conf->bw; j++) {
                    if(kdiff<=0)
                        mvaddch(height-k/8, xoffset+j, ' ');
                    else
                        mvaddch(height-k/8, xoffset+j, 0x40 + kdiff);
                }
            }
        }
    }
    // output: check if terminal has been resized
    //if (!is_tty) {
    //    if (x_axis_info)
    //        terminal_height++;
    //    if (LINES != terminal_height || COLS != terminal_width) {
    //        return TERMINAL_RESIZED;
    //        if (x_axis_info)
    //            terminal_height--;
    //    }
    //}

    // Compute how much of the screen we possibly need to update ahead-of-time.
    //int max_update_y = 0;
    //for (int bar = 0; bar < bars; bar++) {
    //    max_update_y = MAX(max_update_y, MAX(f[bar], flastd[bar]));
    //}

    //max_update_y = (max_update_y + num_bar_heights) / num_bar_heights;

    //for (int y = 0; y < max_update_y; y++) {
    //    if (conf->gradients) {
    //        change_colors(y, height);
    //    }

    //    for (int bar = 0; bar < bars; bar++) {
    //        if (f[bar] == flastd[bar]) {
    //            continue;
    //        }

    //        int cur_col = bar * conf->bw + bar * conf->bs + rest;
    //        int f_cell = (f[bar] - 1) / num_bar_heights;
    //        int f_last_cell = (flastd[bar] - 1) / num_bar_heights;

    //        if (f_cell >= y) {
    //            int bar_step;

    //            if (f_cell == y) {
    //                // The "cap" of the bar occurs at this [y].
    //                bar_step = (f[bar] - 1) % num_bar_heights;
    //            } else if (f_last_cell <= y) {
    //                // The bar is full at this [y].
    //                bar_step = num_bar_heights - 1;
    //            } else {
    //                // No update necessary since last frame.
    //                continue;
    //            }

    //            for (int col = cur_col, i = 0; i < conf->bw; i++, col++) {
    //                //if (is_tty) {
    //                    mvaddch(height - y, col, 0x41 + bar_step);
    //                //} else {
    //                    //mvaddwstr(height - y, col, bar_heights[bar_step]);
    //                //}
    //            }
    //        } else if (f_last_cell >= y) {
    //            // This bar was taller during the last frame than during this frame, so
    //            // clear the excess characters.
    //            for (int col = cur_col, i = 0; i < conf->bw; i++, col++) {
    //                mvaddch(height - y, col, ' ');
    //            }
    //        }
    //    }
    //}


    refresh();
    return 0;
}

EXP_FUNC void wavaOutputLoadConfig(WAVA *wava) {
    // noop
}
