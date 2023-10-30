#ifndef RENDER_H
#define RENDER_H

/* Function Prototypes */
void render_all(void);
void refresh_cell(int, int);
void mark_refresh(int, int);
void render_map(void);
void render_all_actors(void);
void clear_actors(void);
int switch_viewmode(void);

#define mark_refresh(x, y) (g.levmap[x][y].refresh = 1)

#endif