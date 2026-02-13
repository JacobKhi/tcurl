#include "core/request_thread.h"
#include "state.h"
#include "core/http.h"
#include "core/textbuf.h"
#include "core/format.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void *request_thread(void *arg) {
    AppState *s = (AppState *)arg;

    s->response_scroll = 0;

    free(s->response.body);
    s->response.body = NULL;

    free(s->response.body_view);
    s->response.body_view = NULL;

    s->response.status = 0;

    char *payload = tb_to_string(&s->body);
    if (!payload) {
        s->response.body_view = (char *)malloc(64);
        if (s->response.body_view) {
            snprintf(s->response.body_view, 64, "Error: out of memory (payload)");
        }
        s->is_request_in_flight = 0;
        return NULL;
    }

    (void)http_request(s->url, s->method, payload, &s->response);

    free(payload);

    if (s->response.body) {
        char *pretty = json_pretty_print(s->response.body);
        if (pretty) {
            s->response.body_view = pretty;
        } else {
            s->response.body_view = strdup(s->response.body);
        }
    } else {
        s->response.body_view = strdup("Request failed (no response body).");
    }

    s->is_request_in_flight = 0;
    return NULL;
}