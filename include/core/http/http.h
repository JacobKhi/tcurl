#pragma once
#include "state.h"

int http_request(
    const char *url,
    HttpMethod method,
    const char *body,
    const TextBuffer *headers,
    HttpResponse *out
);
