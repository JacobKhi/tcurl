#include "core/http/request_thread.h"
#include "state.h"
#include "core/http/http.h"
#include "core/storage/history.h"
#include "core/storage/history_persistence.h"
#include "core/config/env.h"
#include "core/text/textbuf.h"
#include "core/format/format.h"
#include "core/http/request_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail_with_error(AppState *s, const char *msg) {
    app_state_lock(s);
    free(s->response.response.body);
    free(s->response.response.body_view);
    free(s->response.response.error);
    s->response.response.body = NULL;
    s->response.response.body_view = NULL;
    s->response.response.error = strdup(msg ? msg : "Unknown error");
    s->response.is_request_in_flight = 0;
    app_state_unlock(s);
}

static void fail_with_missing_var(AppState *s, const char *name) {
    if (!name) {
        fail_with_error(s, "Missing variable in request template");
        return;
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Missing variable: %s", name);
    fail_with_error(s, msg);
}

void *request_thread(void *arg) {
    AppState *s = arg;

    RequestSnapshot snap;
    if (request_snapshot_build(s, &snap) != 0) {
        fail_with_error(s, "Out of memory building request snapshot");
        return NULL;
    }

    char *missing = NULL;
    char *url = NULL;
    char *payload = NULL;
    char *headers_text = NULL;

    app_state_lock(s);
    url = env_expand_template(&s->config.envs, snap.url, &missing);
    if (url) payload = env_expand_template(&s->config.envs, snap.body_text, &missing);
    if (url && payload) headers_text = env_expand_template(&s->config.envs, snap.headers_text, &missing);
    app_state_unlock(s);

    if (!url) {
        request_snapshot_free(&snap);
        if (missing) {
            fail_with_missing_var(s, missing);
            free(missing);
        } else {
            fail_with_error(s, "Out of memory resolving URL template");
        }
        return NULL;
    }

    if (!payload) {
        free(url);
        request_snapshot_free(&snap);
        if (missing) {
            fail_with_missing_var(s, missing);
            free(missing);
        } else {
            fail_with_error(s, "Out of memory resolving body template");
        }
        return NULL;
    }

    if (!headers_text) {
        free(url);
        free(payload);
        request_snapshot_free(&snap);
        if (missing) {
            fail_with_missing_var(s, missing);
            free(missing);
        } else {
            fail_with_error(s, "Out of memory resolving headers template");
        }
        return NULL;
    }

    TextBuffer resolved_headers;
    tb_init(&resolved_headers);
    tb_set_from_string(&resolved_headers, headers_text);

    HttpResponse response_local;
    memset(&response_local, 0, sizeof(response_local));
    http_request(
        url,
        snap.method,
        payload,
        &resolved_headers,
        &response_local
    );

    tb_free(&resolved_headers);
    free(url);
    free(payload);
    free(headers_text);

    if (response_local.body) {
        char *pretty = json_pretty_print(response_local.body);
        if (pretty) {
            response_local.body_view = pretty;
            response_local.is_json = 1;
        } else {
            response_local.body_view = strdup(response_local.body);
            response_local.is_json = 0;
        }
    }

    app_state_lock(s);

    s->response.scroll = 0;
    free(s->response.response.body);
    free(s->response.response.body_view);
    free(s->response.response.error);
    s->response.response.body = response_local.body;
    s->response.response.body_view = response_local.body_view;
    s->response.response.error = response_local.error;
    s->response.response.status = response_local.status;
    s->response.response.elapsed_ms = response_local.elapsed_ms;
    s->response.response.is_json = response_local.is_json;

    if (s->history.history) {
        TextBuffer hist_body;
        TextBuffer hist_headers;
        tb_init(&hist_body);
        tb_init(&hist_headers);
        tb_set_from_string(&hist_body, snap.body_text);
        tb_set_from_string(&hist_headers, snap.headers_text);

        history_push(
            s->history.history,
            snap.method,
            snap.url,
            &hist_body,
            &hist_headers,
            &s->response.response
        );

        tb_free(&hist_body);
        tb_free(&hist_headers);

        int count_after_push = s->history.history->count;
        history_trim_oldest(s->history.history, s->history.max_entries);

        int save_rc = 0;
        if (s->history.path) {
            if (s->history.history->count < count_after_push) {
                save_rc = history_storage_save(s->history.history, s->history.path);
            } else {
                save_rc = history_storage_append_last(s->history.history, s->history.path);
                if (save_rc != 0) {
                    save_rc = history_storage_save(s->history.history, s->history.path);
                }
            }
        }
        s->history.last_save_error = save_rc;
    } else {
        s->history.last_save_error = 0;
    }

    s->response.is_request_in_flight = 0;
    app_state_unlock(s);

    request_snapshot_free(&snap);
    return NULL;
}
