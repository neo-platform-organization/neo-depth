#ifndef NEO_DEPTH_BINDINGS_H
#define NEO_DEPTH_BINDINGS_H
/* Private names of the selected implementation. Public calls from the surface
   go through its validated ABI table instead of binding these symbols. */
#define neo_platform_allocate neo_depth_allocate
#define neo_platform_malloc neo_depth_malloc
#define neo_platform_calloc neo_depth_calloc
#define neo_platform_realloc neo_depth_realloc
#define neo_platform_free neo_depth_free
#define neo_native_execute neo_depth_execute
#define neo_native_worker neo_depth_worker
#define neo_native_begin neo_depth_begin
#define neo_native_end neo_depth_end
#define neo_native_worker_id neo_depth_worker_id
#define neo_native_event_platform neo_depth_events
#define neo_platform_services neo_depth_services
#define neo_platform_services_selftest neo_depth_selftest
#define neo_console_open neo_depth_console_open
#define neo_console_close neo_depth_console_close
#define neo_platform_read_line neo_depth_read_line
#define neo_platform_arguments neo_depth_arguments
#define neo_platform_file_open neo_depth_file_open
#endif
