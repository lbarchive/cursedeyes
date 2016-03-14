// cursedeyes, like Xeyes but for terminal
// Written by Yu-Jie Lin in 2016
// This code has been placed in the public domain, or via UNLICENSE.

#include <curses.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <termcap.h>
#include <time.h>


#define STR(s)  XSTR(s)
#define XSTR(s) #s

#ifndef PROGRAM
#define PROGRAM  "cursedeyes"
#endif
#ifndef VERSION
#define VERSION  "unknown"
#endif

#define HELP  "Usage: " STR(PROGRAM) " [OPTIONS]\n\
Like Xeyes, but for terminal\n\
\n\
Options:\n\
\n\
  -h        display this help message\n\
  -v        display version string\n\
\n"


int SCREEN_W = 0;
int SCREEN_H = 0;
mmask_t mmask;
struct {
  int x, y, lx, ly;
  int r;
  double a;
} eyes[2];


void
signal_handler(int sig)
{
  endwin();

  signal(sig, SIG_DFL);
  raise(sig);
}


void
new_eyes()
{
  int max = (SCREEN_H < SCREEN_W) ? SCREEN_H : SCREEN_W;
  int half = max / 2;
  int r = half * 0.9;

  for (int i = 0; i < 2; i++)
  {
    eyes[i].x = SCREEN_W / 2 + half * (i * 2 - 1);
    eyes[i].y = SCREEN_H / 2;
    eyes[i].lx = -1;
    eyes[i].ly = -1;
    eyes[i].r = r;
  }
}


/* Adopted from https://en.wikipedia.org/wiki/Midpoint_circle_algorithm#Example */
void
draw_circle(int x0, int y0, int radius, int c)
{
  int x = radius;
  int y = 0;
  int decisionOver2 = 1 - x;   // Decision criterion divided by 2 evaluated at x=r, y=0

  attrset(COLOR_PAIR(c));
  while (y <= x)
  {
    for (int d = 0; d < x; d++)
    {
      mvaddch( y + y0,  d + x0, ' '); // Octant 1
      mvaddch( y + y0, -d + x0, ' '); // Octant 4
      mvaddch(-y + y0, -d + x0, ' '); // Octant 5
      mvaddch(-y + y0,  d + x0, ' '); // Octant 7
      refresh();
    }
    for (int d = 0; d < y; d++)
    {
      mvaddch( x + y0,  d + x0, ' '); // Octant 2
      mvaddch( x + y0, -d + x0, ' '); // Octant 3
      mvaddch(-x + y0, -d + x0, ' '); // Octant 6
      mvaddch(-x + y0,  d + x0, ' '); // Octant 8
      refresh();
    }

    y++;
    if (decisionOver2 <= 0)
    {
      decisionOver2 += 2 * y + 1;   // Change in decision criterion for y -> y+1
    }
    else
    {
      x--;
      decisionOver2 += 2 * (y - x) + 1;   // Change for y -> y+1, x -> x-1
    }
  }
}


void
draw_eyes(int nx, int ny)
{
  for (int i = 0; i < 2; i++)
  {
    double r  = eyes[i].r * 0.7;
    double r1 = eyes[i].r * 0.2;
    double a;

    r1 = (r1 < 1) ? 1 : r1;

    if (eyes[i].lx != -1)
    {
      draw_circle(eyes[i].lx, eyes[i].ly, r1, 1);
    }

    if (pow(nx - eyes[i].x, 2) + pow(ny - eyes[i].y, 2) < pow(r, 2))
    {
      eyes[i].lx = nx;
      eyes[i].ly = ny;
    }
    else
    {
      a = eyes[i].a = atan2(ny - eyes[i].y, nx - eyes[i].x);
      eyes[i].lx = eyes[i].x + r * cos(a);
      eyes[i].ly = eyes[i].y + r * sin(a);
    }
    draw_circle(eyes[i].lx, eyes[i].ly, r1, 2);
  }
}


void
resize(void)
{
  getmaxyx(stdscr, SCREEN_H, SCREEN_W);

  clear();
  new_eyes();
  for (int i = 0; i < 2; i++)
  {
    draw_circle(eyes[i].x, eyes[i].y, eyes[i].r, 1);
  }
}


void
init_colors(void)
{
  if (!has_colors())
  {
    return;
  }

  use_default_colors();
  start_color();
  init_pair(1, -1, COLOR_WHITE);
  init_pair(2, -1, -1);
}

int
main(int argc, char *argv[])
{
  int opt;
  while ((opt = getopt(argc, argv, "hv")) != -1)
  {
    switch (opt)
    {
      case 'h':
        printf(HELP);
        endwin();
        return EXIT_SUCCESS;
      case 'v':
        printf("%s %s\n", STR(PROGRAM), STR(VERSION));
        printf("built on %s\n", __DATE__);
        return EXIT_SUCCESS;
    }
  }

  /******************/
  /* check XM entry */
  /******************/

  // ncurses' tgetent ignores bp and we don't need that anyway
  if (tgetent(NULL, getenv("TERM")) != 1)
  {
    fprintf(stderr, "Failed to run tgetent\n");
    return EXIT_FAILURE;
  }
  // we don't actually need to check XM content, as long as being returned,
  // that is good enough
  if (tgetstr("XM", NULL) == NULL)
  {
    fprintf(stderr, "No XM entry set in %s, try with TERM=xterm-1003\n", getenv("TERM"));
    return EXIT_FAILURE;
  }

  signal(SIGINT, signal_handler);
  srand(time(NULL));

  initscr();
  keypad(stdscr, TRUE);
  mousemask(REPORT_MOUSE_POSITION, &mmask);
  nonl();
  cbreak();
  noecho();
  timeout(0);
  curs_set(0);
  init_colors();
  resize();
  
  struct timespec req = {
    .tv_sec = 0,
    .tv_nsec = 10000000   // 10ms
  };

  while (TRUE)
  {
    int c = getch();
    MEVENT event;

    switch (c)
    {
      case KEY_RESIZE:
        resize();
        break;
      case KEY_MOUSE:
        if (getmouse(&event) != OK)
        {
          mvprintw(0, 0, "getmouse error!");
          break;
        }
        draw_eyes(event.x, event.y);
        break;
      case 'q':
      case 'Q':
        endwin();
        return EXIT_SUCCESS;
    }

    nanosleep(&req, NULL);
  }
}
