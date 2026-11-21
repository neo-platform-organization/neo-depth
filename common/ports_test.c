#include "ports.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>

#define NEO_PORT_CHECK(value) do { if (!(value)) { fprintf(stderr,"port test line %d: %s (%s)\n",__LINE__,#value,failure ? failure : "no send error"); goto done; } } while (0)
static const char *neo_ports_drain(neo_ports *ports, neo_session *session, const neo_code_platform *platform) {
    for (unsigned i=0; i<64 && neo_ports_pending(ports); ++i) {
        const char *failure=neo_ports_poll(ports,session,platform);
        if (failure) { return failure; }
    }
    return neo_ports_pending(ports) ? "I/O did not finish" : NULL;
}
static bool neo_ports_contents(neo_services *services, neo_handle handle, const char *expected) {
    uint64_t id=0; neo_completion completion;
    if (neo_services_submit(services,handle,NEO_IO_READ,NULL,64,0,&id)!=NEO_OK) { return false; }
    neo_services_poll(services);
    if (neo_services_next(services,&completion)!=NEO_OK) { return false; }
    bool passed=completion.id==id && completion.status==NEO_OK && completion.length==strlen(expected) &&
        !memcmp(completion.bytes,expected,completion.length);
    neo_services_release(services,id); return passed;
}
bool neo_ports_selftest(const neo_code_platform *platform) {
    static const char source[]=
        "(warp 1 (entry client idle)"
        " (object client (package app) (slot status 0) (slot length 0) (slot blob #) (slot message \"old\")"
        "  (knight add a b (+ a b)) (handler idle 0)"
        "  (handler start (io-write output self done \"abc\"))"
        "  (handler read (io-read input self done 31))"
        "  (handler bad (do (set self status 99) (io-write output self done \"discard\") (call self add 2147483647 1)))"
        "  (handler batch (do (io-write output self done \"first\") (io-write output self done \"second\")))"
        "  (handler denied-batch (do (set self status 99) (io-write output self done \"discard\") (io-write missing self done \"no\")))"
        "  (handler copy (do (io-write output self done (get self message)) (set-text self message \"changed\")))"
        "  (handler done payload status length request (do (set-bytes self blob payload) (set self status status) (set self length length)))"
        "  (handler status (get self status)) (handler length (get self length))"
        "  (handler arg (text-length (argument 0))) (handler argc (argument-count)))"
        " (object outsider (package other) (handler try (io-write output self done \"no\"))"
        "  (handler argc (argument-count)) (handler arg (text-length (argument 0)))"
        "  (handler done payload status length request 0)))";
    neo_services *services=NULL; neo_session *session=NULL;
    neo_ports ports={0}; neo_handle output, input; int32_t result=0; bool passed=false;
    const char *failure=NULL;
    NEO_PORT_CHECK(neo_services_headless(&services)==NEO_OK); ports.services=services;
    NEO_PORT_CHECK(!neo_session_open(source,&session));
    NEO_PORT_CHECK(neo_services_memory_stream(services,NEO_RIGHT_READ|NEO_RIGHT_WRITE,NULL,0,&output)==NEO_OK);
    NEO_PORT_CHECK(neo_services_memory_stream(services,NEO_RIGHT_READ,"input",5,&input)==NEO_OK);
    NEO_PORT_CHECK(neo_ports_bind(&ports,"app","output",output,NEO_RIGHT_WRITE)==NEO_OK);
    NEO_PORT_CHECK(neo_ports_bind(&ports,"app","input",input,NEO_RIGHT_READ)==NEO_OK);
    neo_session_io(session,neo_ports_submit,&ports);
    const char *arguments[]={"\"hello\""};
    NEO_PORT_CHECK(!neo_session_arguments(session,"app",arguments,1));
    NEO_PORT_CHECK(!neo_session_send(session,"client","arg",platform,&result) && result==5);
    NEO_PORT_CHECK(!neo_session_send(session,"outsider","argc",platform,&result) && result==0);
    failure=neo_session_send(session,"outsider","arg",platform,&result);
    NEO_PORT_CHECK(failure && !strcmp(failure,"program argument index out of bounds"));
    failure=neo_session_send(session,"outsider","try",platform,&result);
    NEO_PORT_CHECK(failure && !strcmp(failure,"I/O channel access denied") && !neo_ports_pending(&ports));
    NEO_PORT_CHECK(!neo_session_send(session,"client","start",platform,&result));
    NEO_PORT_CHECK(!neo_ports_drain(&ports,session,platform));
    NEO_PORT_CHECK(neo_ports_contents(services,output,"abc"));
    failure=neo_session_send(session,"client","bad",platform,&result);
    NEO_PORT_CHECK(failure && !strcmp(failure,"integer overflow") && !neo_ports_pending(&ports));
    failure=neo_session_send(session,"client","denied-batch",platform,&result);
    NEO_PORT_CHECK(failure && !strcmp(failure,"I/O channel access denied") && !neo_ports_pending(&ports));
    NEO_PORT_CHECK(!neo_session_send(session,"client","status",platform,&result) && result==0);
    NEO_PORT_CHECK(!neo_session_send(session,"client","batch",platform,&result));
    NEO_PORT_CHECK(!neo_ports_drain(&ports,session,platform));
    NEO_PORT_CHECK(neo_ports_contents(services,output,"firstsecond"));
    NEO_PORT_CHECK(!neo_session_send(session,"client","copy",platform,&result));
    NEO_PORT_CHECK(!neo_ports_drain(&ports,session,platform));
    NEO_PORT_CHECK(neo_ports_contents(services,output,"old"));
    NEO_PORT_CHECK(!neo_session_send(session,"client","read",platform,&result));
    NEO_PORT_CHECK(!neo_ports_drain(&ports,session,platform));
    NEO_PORT_CHECK(!neo_session_send(session,"client","length",platform,&result) && result==5);
    NEO_PORT_CHECK(!neo_session_send(session,"client","read",platform,&result));
    NEO_PORT_CHECK(!neo_ports_drain(&ports,session,platform));
    NEO_PORT_CHECK(!neo_session_send(session,"client","status",platform,&result) && result==NEO_EOF);
    NEO_PORT_CHECK(!neo_session_send(session,"client","start",platform,&result));
    NEO_PORT_CHECK(neo_ports_unbind(&ports,"app","output")==NEO_OK);
    NEO_PORT_CHECK(neo_ports_bind(&ports,"app","output",output,NEO_RIGHT_WRITE)==NEO_OK);
    NEO_PORT_CHECK(!neo_ports_drain(&ports,session,platform));
    NEO_PORT_CHECK(!neo_session_send(session,"client","status",platform,&result) && result==NEO_DENIED);
    /* A queued request cannot execute against definitions published later. */
    NEO_PORT_CHECK(!neo_session_send(session,"client","start",platform,&result));
    NEO_PORT_CHECK(!neo_session_reload(&session,1,source));
    failure=neo_ports_poll(&ports,session,platform);
    NEO_PORT_CHECK(failure && !strcmp(failure,"I/O completion targets a stale graph revision"));
    NEO_PORT_CHECK(!neo_session_send(session,"client","arg",platform,&result) && result==5);
    /* The lower transport may report partial writes; one graph completion must
       account for the entire queued payload without reordering subsequent I/O. */
    neo_script_step partial[]={
        {.operation=NEO_IO_WRITE,.status=NEO_OK,.length=1,.bytes={'a'}},
        {.operation=NEO_IO_WRITE,.status=NEO_OK,.length=2,.bytes={'b','c'}}};
    neo_handle fragmented;
    NEO_PORT_CHECK(neo_services_script_stream(services,partial,2,NEO_RIGHT_WRITE,&fragmented)==NEO_OK);
    NEO_PORT_CHECK(neo_ports_unbind(&ports,"app","output")==NEO_OK);
    NEO_PORT_CHECK(neo_ports_bind(&ports,"app","output",fragmented,NEO_RIGHT_WRITE)==NEO_OK);
    NEO_PORT_CHECK(!neo_session_send(session,"client","start",platform,&result));
    NEO_PORT_CHECK(!neo_ports_drain(&ports,session,platform));
    NEO_PORT_CHECK(!neo_session_send(session,"client","length",platform,&result) && result==3);
    NEO_PORT_CHECK(!neo_session_send(session,"client","status",platform,&result) && result==NEO_OK);
    /* Shutdown cancels accepted but undelivered work without invoking code. */
    NEO_PORT_CHECK(!neo_session_send(session,"client","read",platform,&result));
    neo_ports_close(&ports);
    NEO_PORT_CHECK(!neo_ports_pending(&ports));
    passed=true;
done:
    neo_ports_close(&ports); neo_session_close(session); neo_services_close(services);
    if (passed) { puts("PASS graph I/O: arguments, scoped grants, batch rollback, payload copies, ordered/partial writes, EOF, revocation, stale completions, and shutdown"); }
    return passed;
}
