#pragma once

#include "core/textbuf.h"

typedef struct HttpResponse HttpResponse;

typedef struct HistoryItem {
    int method;
    char *url;
    char *body;
    char *headers;

    long status;
    char *response_body;
    char *response_body_view;
    double elapsed_ms;
    int is_json;
} HistoryItem;

typedef struct History {
    HistoryItem *items;
    int count;
    int capacity;
} History;

void history_init(History *h);
void history_free(History *h);

void history_push(
    History *h,
    int method,
    const char *url,
    const TextBuffer *body,
    const TextBuffer *headers,
    const HttpResponse *response
);

HistoryItem *history_get(History *h, int index);
