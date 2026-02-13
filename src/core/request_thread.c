#include "core/request_thread.h"
#include "state.h"
#include "core/http.h"
#include "core/history.h"
#include "core/history_storage.h"
#include "core/env.h"
#include "core/textbuf.h"
#include "core/format.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail_with_error(AppState *s, const char *msg) {
    s->response.error = strdup(msg ? msg : "Unknown error");
    s->is_request_in_flight = 0;
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

    s->response_scroll = 0;

    free(s->response.body);
    free(s->response.body_view);
    free(s->response.error);

    s->response.body = NULL;
    s->response.body_view = NULL;
    s->response.error = NULL;
    s->response.status = 0;
    s->response.elapsed_ms = 0.0;
    s->response.is_json = 0;

    char *payload_template = tb_to_string(&s->body);
    if (!payload_template) {
        fail_with_error(s, "Out of memory building request body");
        return NULL;
    }

    char *headers_template = tb_to_string(&s->headers);
    if (!headers_template) {
        free(payload_template);
        fail_with_error(s, "Out of memory building request headers");
        return NULL;
    }

    char *missing = NULL;
    char *url = env_expand_template(&s->envs, s->url, &missing);
    if (!url) {
        free(payload_template);
        free(headers_template);
        if (missing) {
            fail_with_missing_var(s, missing);
            free(missing);
        } else {
            fail_with_error(s, "Out of memory resolving URL template");
        }
        return NULL;
    }

    char *payload = env_expand_template(&s->envs, payload_template, &missing);
    if (!payload) {
        free(url);
        free(payload_template);
        free(headers_template);
        if (missing) {
            fail_with_missing_var(s, missing);
            free(missing);
        } else {
            fail_with_error(s, "Out of memory resolving body template");
        }
        return NULL;
    }

    char *headers_text = env_expand_template(&s->envs, headers_template, &missing);
    if (!headers_text) {
        free(url);
        free(payload);
        free(payload_template);
        free(headers_template);
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

    http_request(
        url,
        s->method,
        payload,
        &resolved_headers,
        &s->response
    );

    tb_free(&resolved_headers);
    free(url);
    free(payload);
    free(headers_text);
    free(payload_template);
    free(headers_template);

    if (s->response.body) {
        char *pretty = json_pretty_print(s->response.body);
        if (pretty) {
            s->response.body_view = pretty;
            s->response.is_json = 1;
        } else {
            s->response.body_view = strdup(s->response.body);
            s->response.is_json = 0;
        }
    }

    if (s->history) {
        history_push(
            s->history,
            s->method,
            s->url,
            &s->body,
            &s->headers,
            &s->response
        );
        history_trim_oldest(s->history, s->history_max_entries);
        if (s->history_path) {
            (void)history_storage_save(s->history, s->history_path);
        }
    }

    s->is_request_in_flight = 0;
    return NULL;
}
