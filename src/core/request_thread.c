#include "core/request_thread.h"
#include "state.h"
#include "core/http.h"
#include "core/textbuf.h"
#include <stdlib.h>
#include <stdio.h>

void *request_thread(void *arg) {
    AppState *s = (AppState *)arg;

    free(s->response.body);
    s->response.body = NULL;
    s->response.status = 0;

    char *payload = tb_to_string(&s->body);
    if (!payload) {
        s->response.body = (char *)malloc(32);
        if (s->response.body) {
            s->response.status = 0;
            s->response.body[0] = '\0';
            snprintf(s->response.body, 32, "Error: out of memory");
        }
        s->is_request_in_flight = 0;
        return NULL;
    }

    (void)http_request(s->url, s->method, payload, &s->response);

    free(payload);

    s->is_request_in_flight = 0;
    return NULL;
}
