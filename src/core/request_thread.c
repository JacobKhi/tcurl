#include "core/request_thread.h"
#include "state.h"
#include "core/http.h"
#include "core/history.h"
#include "core/history_storage.h"
#include "core/textbuf.h"
#include "core/format.h"
#include <stdlib.h>
#include <string.h>

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

    char *payload = tb_to_string(&s->body);
    if (!payload) {
        s->response.error = strdup("Out of memory building request body");
        s->is_request_in_flight = 0;
        return NULL;
    }

    http_request(
        s->url,
        s->method,
        payload,
        &s->headers,
        &s->response
    );

    free(payload);

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
