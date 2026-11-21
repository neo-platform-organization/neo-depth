#ifndef NEO_PORTS_H
#define NEO_PORTS_H
#include "jit.h"
#include "services.h"
enum { NEO_PORT_CHANNELS=32, NEO_PORT_REQUESTS=32 };
typedef struct {
    bool active;
    char package[64], name[64];
    neo_handle handle;
    unsigned rights;
    uint64_t generation;
} neo_port_channel;
typedef struct {
    bool active, write;
    uint32_t id;
    uint64_t native, revision, generation, deadline;
    size_t channel, length, offset;
    char object[64], selector[64];
    unsigned char bytes[64];
} neo_port_request;
typedef struct {
    /* A dedicated service instance is borrowed for the lifetime of this bridge.
       The caller closes it after neo_ports_close. Calls are owner-serialized. */
    neo_services *services;
    neo_port_channel channels[NEO_PORT_CHANNELS];
    neo_port_request requests[NEO_PORT_REQUESTS];
    uint32_t sequence;
} neo_ports;
neo_status neo_ports_bind(neo_ports *, const char *, const char *, neo_handle, unsigned);
neo_status neo_ports_unbind(neo_ports *, const char *, const char *);
const char *neo_ports_submit(void *, uint64_t, const neo_io_message *, size_t);
const char *neo_ports_poll(neo_ports *, neo_session *, const neo_code_platform *);
bool neo_ports_pending(const neo_ports *);
void neo_ports_close(neo_ports *);
/* Convert a small raw UTF-8 argument to a validated inert text token. */
const char *neo_argument_literal(const char *, char [64]);
bool neo_ports_selftest(const neo_code_platform *);
#endif
