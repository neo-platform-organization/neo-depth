#include "bindings.h"
#include "depth.h"
#if defined(NEO_FREESTANDING)
#define NEO_PROVIDER_NAME "x86_64"
#elif defined(_WIN32)
#define NEO_PROVIDER_NAME "x86_64/Windows"
#elif defined(__linux__)
#define NEO_PROVIDER_NAME "x86_64/Linux"
#else
#error No depth provider is registered for this target
#endif
const neo_depth_plugin *neo_depth_provider(void) {
    static const neo_depth_plugin provider={
        .abi=NEO_DEPTH_ABI,.bytes=sizeof(neo_depth_plugin),.name=NEO_PROVIDER_NAME,
        .allocate=neo_platform_allocate,.malloc_bytes=neo_platform_malloc,.calloc_bytes=neo_platform_calloc,
        .realloc_bytes=neo_platform_realloc,.free_bytes=neo_platform_free,.execute=neo_native_execute,
        .worker=neo_native_worker,.begin=neo_native_begin,.end=neo_native_end,.worker_id=neo_native_worker_id,
        .events=neo_native_event_platform,.services=neo_platform_services,.selftest=neo_platform_services_selftest,
        .console_open=neo_console_open,.console_close=neo_console_close,.read_line=neo_platform_read_line};
    return &provider;
}
