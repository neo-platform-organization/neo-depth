#include "launch.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

int neo_launch_command(const char *command, const char *source, const char *object,
                       const neo_code_platform *platform) {
    neo_session *image=NULL, *minimal=NULL;
    neo_program_config config;
    static char package[65537], from_image[65537], from_minimal[65537];
    const char *failure=neo_session_open(source,&image);
    if (!failure) { failure=neo_session_program_config(image,object,&config); }
    if (!failure && !strcmp(command,"launch-info")) {
        static const char *names[]={"internal","host","headless"};
        printf("program %s\npackage %s\nentry %s\npresentation %s\ninstances %s\n",
            config.object,config.package,config.selector,names[config.presentation],
            config.multiple_instances ? "optional" : "single");
    } else if (!failure) {
        failure=neo_session_package_image(image,object,package,sizeof(package));
        if (!failure && !strcmp(command,"package")) {
            if (puts(package)==EOF) { failure="package output failed"; }
        } else if (!failure && !strcmp(command,"program-check")) {
            failure=neo_session_open(package,&minimal);
            neo_native_begin();
            for (unsigned run=0; !failure && run<2; ++run) {
                int32_t first=0, second=0;
                failure=neo_session_send(image,config.object,config.selector,platform,&first);
                if (!failure) { failure=neo_session_send(minimal,config.object,config.selector,platform,&second); }
                if (!failure && first!=second) { failure="deployment results differ"; }
                if (!failure) { failure=neo_session_package_image(image,object,from_image,sizeof(from_image)); }
                if (!failure) { failure=neo_session_package_image(minimal,object,from_minimal,sizeof(from_minimal)); }
                if (!failure && strcmp(from_image,from_minimal)) { failure="deployment state differs"; }
                if (!failure) { printf("result %" PRId32 "\n",first); }
            }
            neo_native_end();
            if (!failure) { puts("PASS same package behavior and state in full and minimal images"); }
        }
    }
    if (failure) { fprintf(stderr,"neo launch: %s\n",failure); }
    neo_session_close(minimal); neo_session_close(image);
    return failure ? 1 : 0;
}

typedef struct { const char *failure; } neo_program_events;
static void neo_program_completed(void *context, const char *id, const char *failure, int32_t result) {
    neo_program_events *state=context;
    (void)id; (void)result;
    if (failure) { state->failure=failure; }
}
static bool neo_program_events_pending(const neo_events *events) {
    for (size_t i=0; i<NEO_EVENT_LIMIT; ++i) { if (events->items[i].active) { return true; } }
    return false;
}
int neo_program_run(const char *source, const char *object, const char *const *arguments, size_t count,
    bool full, bool print_result, bool repl, bool execute, const neo_code_platform *platform) {
    neo_session *session=NULL;
    neo_services *services=NULL;
    neo_ports ports={0};
    neo_events events={0}; events.platform=neo_native_event_platform();
    neo_program_events completion={0}; events.complete=neo_program_completed; events.completion_context=&completion;
    static char minimal[65537];
    char receiver[64], selector[64], package[64];
    const char *failure=neo_session_open(source,&session);
    if (!failure) { neo_session_entry(session,receiver,selector); }
    if (!failure && object) {
        if (strlen(object)>=sizeof(receiver)) { failure="program name exceeds limit"; }
        else { strcpy(receiver,object); }
    }
    neo_program_config config;
    bool configured=false;
    if (!failure) {
        failure=neo_session_program_config(session,receiver,&config);
        if (!failure) { configured=true; strcpy(selector,config.selector); }
        else if (!object && !strcmp(failure,"program has no entry-selector metadata")) { failure=NULL; }
    }
    if (!failure && configured && !full) {
        failure=neo_session_package_image(session,receiver,minimal,sizeof(minimal));
        if (!failure) { neo_session_close(session); session=NULL; failure=neo_session_open(minimal,&session); }
    }
    if (!failure) { failure=neo_session_object_package(session,receiver,package); }
    char literals[NEO_PROGRAM_ARGUMENTS][64]; const char *encoded[NEO_PROGRAM_ARGUMENTS];
    if (!failure && count>NEO_PROGRAM_ARGUMENTS) { failure="program argument limit exceeded"; }
    for (size_t i=0; !failure && i<count; ++i) {
        failure=neo_argument_literal(arguments[i],literals[i]); encoded[i]=literals[i];
    }
    if (!failure) { failure=neo_session_arguments(session,package,encoded,count); }
    neo_status status=NEO_OK;
    if (!failure) { status=neo_platform_services(false,&services); ports.services=services; }
    for (unsigned stream=0; !failure && status==NEO_OK && stream<3; ++stream) {
        if (repl && stream==0) { continue; } /* The attached REPL owns command input. */
        neo_handle handle; bool terminal; uint32_t columns, rows;
        status=neo_services_standard(services,stream,&handle,&terminal,&columns,&rows);
        if (status==NEO_OK) {
            static const char *names[]={"input","output","error"};
            status=neo_ports_bind(&ports,package,names[stream],handle,stream ? NEO_RIGHT_WRITE : NEO_RIGHT_READ);
        }
    }
    if (!failure && status!=NEO_OK) { failure=neo_status_name(status); }
    if (!failure) { neo_session_io(session,neo_ports_submit,&ports); neo_session_schedule(session,neo_events_schedule,&events); }
    neo_native_begin(); int32_t result=0; int exit_status=0;
    if (!failure && execute) { failure=neo_session_send(session,receiver,selector,platform,&result); }
    if (!failure && print_result && execute) { printf("%" PRId32 "\n",result); fflush(stdout); }
    if (!failure && repl) { exit_status=neo_repl_session(&session,&events,&ports,platform); }
    while (!failure && !repl && (neo_ports_pending(&ports) || neo_program_events_pending(&events))) {
        neo_events_poll(&events,session,platform);
        failure=completion.failure;
        if (!failure) { failure=neo_ports_poll(&ports,session,platform); }
        if (!failure && (neo_ports_pending(&ports) || neo_program_events_pending(&events))) { neo_services_wait(services,5); }
    }
    if (failure) { fprintf(stderr,"neo program: %s\n",failure); exit_status=1; }
    neo_events_close(&events); neo_ports_close(&ports);
    neo_session_close(session); neo_services_close(services); neo_native_end();
    return exit_status;
}
