#ifndef HALO_VITA_RUNTIME_H
#define HALO_VITA_RUNTIME_H
#include <stddef.h>
#include <stdint.h>

#define HALO_VITA_DATA_ROOT "ux0:data/HaloCE/"
struct vita_gamepad_sample {
	uint16_t buttons;
	uint8_t analog[8];
	int16_t lx, ly, rx, ry;
};
int vita_read_gamepad(struct vita_gamepad_sample *sample);
void vita_log(const char *format, ...) __attribute__((format(printf, 1, 2)));
void vita_fatal(const char *reason) __attribute__((noreturn));
uint64_t vita_time_us(void);
void vita_free_memory(uint32_t *user, uint32_t *cdram, uint32_t *phycont);
int halo_vita_core_initialize(void);
int halo_vita_verify_map(const void *header, size_t length, const char *name);
char *halo_vita_vertex_shader(void);
char *halo_vita_pixel_shader(void);
void halo_vita_core_dispose(void);
int vita_graphics_initialize(void);
int vita_graphics_handoff_frame(void);
int vita_graphics_shader_probe(const char *vertex, const char *fragment);
int vita_graphics_copy_probe(void);
void vita_graphics_frame(int maps_valid, int core_valid, int shader_valid);
void vita_graphics_shutdown(void);
int vita_maps_verify(void);
int vita_map_path(const char *name, char *path, size_t capacity);
void vita_graphics_cache_status(int status);
int vita_platform_initialize(void);
int vita_controls_poll(void);
void vita_platform_shutdown(void);
int vita_services_probe(void);
int halo_vita_file_contract_probe(void);
int vita_xapi_file_attributes(const char *xbox_path, uint32_t *attributes, uint32_t *error);
uint32_t vita_xapi_last_error_get(void);
void vita_xapi_last_error_set(uint32_t error);
#endif
