#ifndef PE_WINDOW_H_HEADER_GUARD
#define PE_WINDOW_H_HEADER_GUARD

#include <stdbool.h>

typedef struct peArena peArena;

void pe_window_set_target_fps(int fps);
float pe_window_delta_time(void);

void pe_window_init(peArena *temp_arena, int window_width, int window_height, const char *window_name);
void pe_window_shutdown(void);
bool pe_window_should_quit(void);
void pe_window_frame_begin(void);
void pe_window_frame_end(bool vsync);

void pe_window_platform_init(int window_width, int window_height, const char *window_name);
void pe_window_platform_shutdown(void);

void pe_window_poll_events(void);
void pe_window_swap_buffers(bool vsync);

#endif // PE_WINDOW_H_HEADER_GUARD