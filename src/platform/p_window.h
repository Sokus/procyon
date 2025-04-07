#ifndef P_WINDOW_HEADER_GUARD
#define P_WINDOW_HEADER_GUARD

#include <stdbool.h>

typedef struct pArena pArena;

void p_window_set_target_fps(int fps);
float p_window_delta_time(void);

void p_window_init(pArena *temp_arena, int window_width, int window_height, const char *window_name);
void p_window_shutdown(void);
bool p_window_should_quit(void);
void p_window_frame_begin(void);
void p_window_frame_end(bool vsync);

void p_window_platform_init(int window_width, int window_height, const char *window_name);
void p_window_platform_shutdown(void);

void p_window_poll_events(void);
void p_window_swap_buffers(bool vsync);

#endif // P_WINDOW_HEADER_GUARD
