#include "core/request_thread.h"
#include "state.h"
#include "core/http.h"
#include <stdlib.h>

void *request_thread(void *arg) {
    AppState *s = arg;

    free(s->response.body);
    s->response.body = NULL;
    s->response.status = 0;

    http_request(s->url, s->method, &s->response);

    s->is_request_in_flight = 0;
    return NULL;
}
