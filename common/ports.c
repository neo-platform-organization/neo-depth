#include "ports.h"
#include "literal.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

const char *neo_argument_literal(const char *text, char output[64]) {
    size_t length=1; output[0]='"';
    for (const unsigned char *byte=(const unsigned char *)text; *byte; ++byte) {
        bool escape=*byte=='"' || *byte=='\\' || *byte=='\n' || *byte=='\r' || *byte=='\t';
        if ((*byte<32 || *byte==127) && !escape) { return "unsupported control byte in argument"; }
        if (length+(escape ? 2u : 1u)+2u>64) { return "argument exceeds current text limit"; }
        if (escape) { output[length++]='\\'; }
        output[length++]=*byte=='\n' ? 'n' : *byte=='\r' ? 'r' : *byte=='\t' ? 't' : (char)*byte;
    }
    output[length++]='"'; output[length]=0; return NULL;
}
static size_t neo_ports_find(const neo_ports *ports, const char *package, const char *name) {
    for (size_t i=0; i<NEO_PORT_CHANNELS; ++i) {
        const neo_port_channel *channel=&ports->channels[i];
        if (channel->active && !strcmp(channel->package,package) && !strcmp(channel->name,name)) { return i; }
    }
    return NEO_PORT_CHANNELS;
}
neo_status neo_ports_bind(neo_ports *ports, const char *package, const char *name, neo_handle handle, unsigned rights) {
    if (!ports->services || !package || !name || !*package || !*name || strlen(package)>=64 || strlen(name)>=64 ||
        !rights || (rights & ~(unsigned)(NEO_RIGHT_READ|NEO_RIGHT_WRITE))) { return NEO_INVALID; }
    if (neo_ports_find(ports,package,name)!=NEO_PORT_CHANNELS) { return NEO_CONFLICT; }
    for (size_t i=0; i<NEO_PORT_CHANNELS; ++i) {
        neo_port_channel *channel=&ports->channels[i];
        if (channel->active) { continue; }
        if (channel->generation==UINT64_MAX) { continue; }
        uint64_t generation=channel->generation+1;
        *channel=(neo_port_channel){.active=true,.handle=handle,.rights=rights,.generation=generation};
        strcpy(channel->package,package); strcpy(channel->name,name); return NEO_OK;
    }
    return NEO_EXHAUSTED;
}
neo_status neo_ports_unbind(neo_ports *ports, const char *package, const char *name) {
    size_t index=neo_ports_find(ports,package,name);
    if (index==NEO_PORT_CHANNELS) { return NEO_CLOSED; }
    ports->channels[index].active=false; return NEO_OK;
}
const char *neo_ports_submit(void *context, uint64_t revision, const neo_io_message *messages, size_t count) {
    neo_ports *ports=context;
    size_t available=0, channels[NEO_DEFERRED_LIMIT];
    if (count>NEO_DEFERRED_LIMIT || count>(uint32_t)INT32_MAX-ports->sequence) { return "I/O request limit exceeded"; }
    for (size_t i=0; i<NEO_PORT_REQUESTS; ++i) { if (!ports->requests[i].active) { ++available; } }
    if (available<count) { return "I/O queue full"; }
    for (size_t i=0; i<count; ++i) {
        const neo_io_message *message=&messages[i];
        if (!message->package || !message->channel || !message->object || !message->selector ||
            strlen(message->object)>=64 || strlen(message->selector)>=64 || message->length>64 ||
            (!message->write && message->length>31)) { return "invalid I/O message"; }
        channels[i]=neo_ports_find(ports,message->package,message->channel);
        if (channels[i]==NEO_PORT_CHANNELS || !(ports->channels[channels[i]].rights &
            (message->write ? NEO_RIGHT_WRITE : NEO_RIGHT_READ))) { return "I/O channel access denied"; }
    }
    size_t next=0;
    for (size_t i=0; i<NEO_PORT_REQUESTS && next<count; ++i) {
        neo_port_request *request=&ports->requests[i];
        if (request->active) { continue; }
        const neo_io_message *message=&messages[next];
        size_t channel=channels[next++];
        *request=(neo_port_request){.active=true,.write=message->write,.id=++ports->sequence,
            .revision=revision,.generation=ports->channels[channel].generation,.channel=channel,.length=message->length};
        strcpy(request->object,message->object); strcpy(request->selector,message->selector);
        if (message->write) { memcpy(request->bytes,message->bytes,message->length); }
    }
    return NULL;
}
static const char *neo_ports_deliver(neo_ports *ports, neo_port_request *request, neo_session *session,
    const neo_code_platform *platform, neo_status status, const unsigned char *bytes, size_t length) {
    char object[64], selector[64], literal[64];
    strcpy(object,request->object); strcpy(selector,request->selector);
    uint64_t revision=request->revision;
    int32_t arguments[]={(int32_t)status,(int32_t)(request->write ? request->offset : length),(int32_t)request->id};
    literal[0]='#'; size_t payload=request->write ? 0 : length;
    if (payload>31) { return "I/O completion exceeds current byte limit"; }
    static const char hex[]="0123456789abcdef";
    for (size_t i=0; i<payload; ++i) { literal[1+2*i]=hex[bytes[i]>>4]; literal[2+2*i]=hex[bytes[i]&15]; }
    literal[1+2*payload]=0;
    *request=(neo_port_request){0};
    if (revision!=neo_session_revision(session)) { return "I/O completion targets a stale graph revision"; }
    int32_t result=0;
    (void)ports;
    return neo_session_send_sequence_args(session,object,selector,literal,arguments,3,platform,&result);
}
static bool neo_ports_same_handle(neo_handle a, neo_handle b) {
    return a.owner==b.owner && a.slot==b.slot && a.generation==b.generation && a.kind==b.kind;
}
const char *neo_ports_poll(neo_ports *ports, neo_session *session, const neo_code_platform *platform) {
    uint32_t previous=0, last=ports->sequence;
    for (size_t visited=0; visited<NEO_PORT_REQUESTS; ++visited) {
        neo_port_request *request=NULL;
        for (size_t i=0; i<NEO_PORT_REQUESTS; ++i) {
            neo_port_request *candidate=&ports->requests[i];
            if (candidate->active && candidate->id>previous && candidate->id<=last &&
                (!request || candidate->id<request->id)) { request=candidate; }
        }
        if (!request) { break; } previous=request->id;
        if (request->revision!=neo_session_revision(session)) {
            if (request->native) { neo_services_cancel(ports->services,request->native); }
            else { *request=(neo_port_request){0}; return "I/O completion targets a stale graph revision"; }
        }
        neo_port_channel *channel=&ports->channels[request->channel];
        if (!channel->active || channel->generation!=request->generation) {
            if (request->native) { neo_services_cancel(ports->services,request->native); }
            else {
                const char *failure=neo_ports_deliver(ports,request,session,platform,NEO_DENIED,NULL,0);
                if (failure) { return failure; } continue;
            }
        }
        if (request->native) { continue; }
        bool blocked=false;
        for (size_t i=0; i<NEO_PORT_REQUESTS; ++i) {
            const neo_port_request *other=&ports->requests[i];
            if (other->active && other!=request && other->id<request->id && other->write==request->write &&
                neo_ports_same_handle(ports->channels[other->channel].handle,channel->handle)) { blocked=true; break; }
        }
        if (blocked) { continue; }
        uint64_t now=neo_services_clock(ports->services);
        if (!request->deadline) {
            if (now>UINT64_MAX-UINT64_C(5000000000)) { return "I/O deadline overflow"; }
            request->deadline=now+UINT64_C(5000000000);
        }
        neo_status status=neo_services_submit(ports->services,channel->handle,request->write ? NEO_IO_WRITE : NEO_IO_READ,
            request->write ? request->bytes+request->offset : NULL,request->length-request->offset,
            request->deadline,&request->native);
        if (status!=NEO_OK) {
            const char *failure=neo_ports_deliver(ports,request,session,platform,status,NULL,0);
            if (failure) { return failure; }
        }
    }
    neo_services_poll(ports->services);
    neo_completion completion;
    while (neo_services_next(ports->services,&completion)==NEO_OK) {
        neo_port_request *request=NULL;
        for (size_t i=0; i<NEO_PORT_REQUESTS; ++i) {
            if (ports->requests[i].active && ports->requests[i].native==completion.id) { request=&ports->requests[i]; break; }
        }
        if (!request) { neo_services_release(ports->services,completion.id); return "foreign I/O completion in bridge"; }
        unsigned char bytes[31]; size_t length=completion.length;
        if (!request->write && length>sizeof(bytes)) { neo_services_release(ports->services,completion.id); return "oversized I/O completion"; }
        if (!request->write && length) { memcpy(bytes,completion.bytes,length); }
        neo_status status=completion.status;
        if (request->write) {
            if (length>request->length-request->offset) { status=NEO_INVALID; length=0; }
            request->offset+=length;
        }
        neo_services_release(ports->services,completion.id); request->native=0;
        if (request->write && status==NEO_OK && request->offset<request->length && length) { continue; }
        if (request->write && status==NEO_OK && request->offset<request->length) { status=NEO_IO_ERROR; }
        const char *failure=neo_ports_deliver(ports,request,session,platform,status,bytes,request->write ? 0 : length);
        if (failure) { return failure; }
    }
    return NULL;
}
bool neo_ports_pending(const neo_ports *ports) {
    for (size_t i=0; i<NEO_PORT_REQUESTS; ++i) { if (ports->requests[i].active) { return true; } }
    return false;
}
void neo_ports_close(neo_ports *ports) {
    if (ports->services) { neo_services_shutdown(ports->services,NULL,NULL); }
    memset(ports->requests,0,sizeof(ports->requests)); memset(ports->channels,0,sizeof(ports->channels));
}
