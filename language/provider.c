#include "language.h"
extern const char * neo_depth_lang_neo_run(const char *source, const neo_code_platform *platform, int32_t *result);
extern const char * neo_depth_lang_neo_roundtrip(const char *source, const neo_code_platform *platform, int32_t *result);
extern const char * neo_depth_lang_neo_snapshot(const char *source, const neo_code_platform *platform, char *output, size_t capacity);
extern const char * neo_depth_lang_neo_session_program_config(const neo_session *, const char *, neo_program_config *);
extern void neo_depth_lang_neo_session_entry(const neo_session *, char [64], char [64]);
extern const char * neo_depth_lang_neo_session_package_image(const neo_session *, const char *, char *, size_t);
extern const char * neo_depth_lang_neo_session_arguments(neo_session *, const char *, const char *const *, size_t);
extern void neo_depth_lang_neo_session_io(neo_session *, neo_io_sink *, void *);
extern void neo_depth_lang_neo_session_schedule(neo_session *, neo_deferred_sink *, void *);
extern const char * neo_depth_lang_neo_session_grant_buffer(neo_session *, const char *, neo_buffer_sink *, void *);
extern const char * neo_depth_lang_neo_session_object_package(const neo_session *, const char *, char [64]);
extern const char * neo_depth_lang_neo_session_clone(neo_session **session, uint64_t revision, const char *prototype, const char *name);
extern const char * neo_depth_lang_neo_session_delete(neo_session **session, uint64_t revision, const char *name);
extern const char * neo_depth_lang_neo_session_open(const char *source, neo_session **session);
extern void neo_depth_lang_neo_session_close(neo_session *session);
extern void neo_depth_lang_neo_session_grant_document(neo_session *session, int32_t *document, unsigned rights);
extern void neo_depth_lang_neo_session_document_sink(neo_session *, neo_document_sink *, void *);
extern const char * neo_depth_lang_neo_session_authorize_import(neo_session *session, const char *object, const char *alias, bool allow);
extern const char * neo_depth_lang_neo_session_grant_package_document(neo_session *session, const char *package, int32_t *document, unsigned rights);
extern uint64_t neo_depth_lang_neo_session_revision(const neo_session *session);
extern const char * neo_depth_lang_neo_session_send(neo_session *session, const char *object, const char *selector, const neo_code_platform *platform, int32_t *result);
extern const char * neo_depth_lang_neo_session_send_value(neo_session *session, const char *object, const char *selector, int32_t argument, const neo_code_platform *platform, int32_t *result);
extern const char * neo_depth_lang_neo_session_send_args(neo_session *session, const char *object, const char *selector, const int32_t *arguments, size_t count, const neo_code_platform *platform, int32_t *result);
extern const char * neo_depth_lang_neo_session_receive_sequence(neo_session *session, const char *object, const char *selector, const int32_t *arguments, size_t count, const neo_code_platform *platform, char kind, char result[NEO_TEXT_RESULT_CAPACITY]);
extern const char * neo_depth_lang_neo_session_send_text(neo_session *session, const char *object, const char *selector, const char *literal, const neo_code_platform *platform, int32_t *result);
extern const char * neo_depth_lang_neo_session_send_sequence_args(neo_session *, const char *, const char *, const char *, const int32_t *, size_t, const neo_code_platform *, int32_t *);
extern const char * neo_depth_lang_neo_session_trace(neo_session *session, const char *object, const char *selector, const int32_t *arguments, size_t count, const neo_code_platform *platform, int32_t *result, neo_trace_sink *sink, void *context);
extern const char * neo_depth_lang_neo_session_reload(neo_session **session, uint64_t expected, const char *source);
extern const char * neo_depth_lang_neo_session_snapshot(const neo_session *session, char *output, size_t capacity);
extern const char * neo_depth_lang_neo_session_weft(const neo_session *session, char *text, size_t capacity);
extern const char * neo_depth_lang_neo_session_inspect(const neo_session *session, const char *object, const char *member, neo_inspect_view view, char *text, size_t capacity);
extern const char * neo_depth_lang_neo_session_evaluate_knight(const neo_session *session, uint64_t revision, const char *object, const char *handler, int32_t state, const int32_t *arguments, size_t count, const neo_code_platform *platform, int32_t *next, int32_t *result);
extern const char * neo_depth_lang_neo_session_probe_knight(const neo_session *session, uint64_t revision, const char *object, const char *function, const char *source, const int32_t *arguments, size_t count, const neo_code_platform *platform, int32_t *result);
extern const char * neo_depth_lang_neo_session_publish_knight(neo_session **session, uint64_t revision, const char *object, const char *function, const char *source);
extern const char * neo_depth_lang_neo_session_transform_knight(const neo_session *session, uint64_t revision, const char *object, const char *function, const char *from, const char *to, char *output, size_t capacity);
extern const char * neo_depth_lang_neo_weft_lower(const char *source, char *warp, size_t capacity);
const neo_language_plugin neo_depth_language_implementation={
    .abi=NEO_LANGUAGE_ABI,.bytes=sizeof(neo_language_plugin),
    .neo_run=neo_depth_lang_neo_run,
    .neo_roundtrip=neo_depth_lang_neo_roundtrip,
    .neo_snapshot=neo_depth_lang_neo_snapshot,
    .neo_session_program_config=neo_depth_lang_neo_session_program_config,
    .neo_session_entry=neo_depth_lang_neo_session_entry,
    .neo_session_package_image=neo_depth_lang_neo_session_package_image,
    .neo_session_arguments=neo_depth_lang_neo_session_arguments,
    .neo_session_io=neo_depth_lang_neo_session_io,
    .neo_session_schedule=neo_depth_lang_neo_session_schedule,
    .neo_session_grant_buffer=neo_depth_lang_neo_session_grant_buffer,
    .neo_session_object_package=neo_depth_lang_neo_session_object_package,
    .neo_session_clone=neo_depth_lang_neo_session_clone,
    .neo_session_delete=neo_depth_lang_neo_session_delete,
    .neo_session_open=neo_depth_lang_neo_session_open,
    .neo_session_close=neo_depth_lang_neo_session_close,
    .neo_session_grant_document=neo_depth_lang_neo_session_grant_document,
    .neo_session_document_sink=neo_depth_lang_neo_session_document_sink,
    .neo_session_authorize_import=neo_depth_lang_neo_session_authorize_import,
    .neo_session_grant_package_document=neo_depth_lang_neo_session_grant_package_document,
    .neo_session_revision=neo_depth_lang_neo_session_revision,
    .neo_session_send=neo_depth_lang_neo_session_send,
    .neo_session_send_value=neo_depth_lang_neo_session_send_value,
    .neo_session_send_args=neo_depth_lang_neo_session_send_args,
    .neo_session_receive_sequence=neo_depth_lang_neo_session_receive_sequence,
    .neo_session_send_text=neo_depth_lang_neo_session_send_text,
    .neo_session_send_sequence_args=neo_depth_lang_neo_session_send_sequence_args,
    .neo_session_trace=neo_depth_lang_neo_session_trace,
    .neo_session_reload=neo_depth_lang_neo_session_reload,
    .neo_session_snapshot=neo_depth_lang_neo_session_snapshot,
    .neo_session_weft=neo_depth_lang_neo_session_weft,
    .neo_session_inspect=neo_depth_lang_neo_session_inspect,
    .neo_session_evaluate_knight=neo_depth_lang_neo_session_evaluate_knight,
    .neo_session_probe_knight=neo_depth_lang_neo_session_probe_knight,
    .neo_session_publish_knight=neo_depth_lang_neo_session_publish_knight,
    .neo_session_transform_knight=neo_depth_lang_neo_session_transform_knight,
    .neo_weft_lower=neo_depth_lang_neo_weft_lower
};
