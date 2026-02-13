#include "core/request_snapshot.h"

#include "core/env.h"
#include "core/textbuf.h"

#include <stdlib.h>
#include <string.h>

static char *dup_or_empty(const char *s) {
    return strdup(s ? s : "");
}

int request_snapshot_build_locked(const AppState *s_const, RequestSnapshot *out) {
    if (!s_const || !out) return 1;

    memset(out, 0, sizeof(*out));

    const AppState *s = s_const;
    out->method = s->method;
    out->url = dup_or_empty(s->url);
    out->body_text = tb_to_string(&s->body);
    out->headers_text = tb_to_string(&s->headers);
    out->env_name = dup_or_empty(env_store_active_name(&s->envs));

    if (!out->url || !out->body_text || !out->headers_text || !out->env_name) {
        request_snapshot_free(out);
        return 1;
    }

    return 0;
}

int request_snapshot_build(const AppState *s_const, RequestSnapshot *out) {
    if (!s_const || !out) return 1;
    AppState *s = (AppState *)s_const;
    app_state_lock(s);
    int rc = request_snapshot_build_locked(s_const, out);
    app_state_unlock(s);
    return rc;
}

void request_snapshot_free(RequestSnapshot *s) {
    if (!s) return;
    free(s->url);
    free(s->body_text);
    free(s->headers_text);
    free(s->env_name);
    memset(s, 0, sizeof(*s));
}
