#ifndef NEO_LAUNCH_H
#define NEO_LAUNCH_H
#include "jit.h"
#include "events.h"
#include "ports.h"
/* Inert inspection/export and an explicit execution diagnostic. These do not
   implement the later script lifecycle or graphical presentation manager. */
int neo_launch_command(const char *, const char *, const char *, const neo_code_platform *);
/* Borrow the same live image and event queues. No new image/app is instantiated. */
int neo_repl_session(neo_session **, neo_events *, neo_ports *, const neo_code_platform *);
int neo_program_run(const char *, const char *, const char *const *, size_t, bool, bool, bool, bool,
                    const neo_code_platform *);
#endif
