#include "core/history.h"
#include "state.h"

#include <stdlib.h>
#include <string.h>

static char *dup_or_empty(const char *s) {
    return s ? strdup(s) : strdup("");
}

static char *dup_or_null(const char *s) {
    return s ? strdup(s) : NULL;
}

static void free_history_item(HistoryItem *it) {
    if (!it) return;
    free(it->url);
    free(it->body);
    free(it->headers);
    free(it->response_body);
    free(it->response_body_view);
}

void history_init(History *h) {
    if (!h) return;

    h->items = NULL;
    h->count = 0;
    h->capacity = 0;
}

void history_free(History *h) {
    if (!h) return;

    for (int i = 0; i < h->count; i++) {
        free_history_item(&h->items[i]);
    }

    free(h->items);
    h->items = NULL;
    h->count = 0;
    h->capacity = 0;
}

void history_push(
    History *h,
    int method,
    const char *url,
    const TextBuffer *body,
    const TextBuffer *headers,
    const HttpResponse *response
) {
    if (!h) return;

    if (h->count == h->capacity) {
        int newcap = h->capacity ? h->capacity * 2 : 8;
        HistoryItem *n = realloc(h->items, (size_t)newcap * sizeof(*n));
        if (!n) return;
        h->items = n;
        h->capacity = newcap;
    }

    HistoryItem *it = &h->items[h->count];
    memset(it, 0, sizeof(*it));

    it->method = method;
    it->url = dup_or_empty(url);

    char *body_str = tb_to_string(body);
    it->body = dup_or_empty(body_str);
    free(body_str);

    char *headers_str = tb_to_string(headers);
    it->headers = dup_or_empty(headers_str);
    free(headers_str);

    if (response) {
        it->status = response->status;
        it->elapsed_ms = response->elapsed_ms;
        it->is_json = response->is_json;
        it->response_body = dup_or_null(response->body);
        it->response_body_view = dup_or_null(response->body_view);
    }

    h->count++;
}

HistoryItem *history_get(History *h, int index) {
    if (!h || index < 0 || index >= h->count) return NULL;
    return &h->items[index];
}

void history_trim_oldest(History *h, int max_entries) {
    if (!h || max_entries < 0) return;
    if (h->count <= max_entries) return;

    int drop = h->count - max_entries;
    for (int i = 0; i < drop; i++) {
        free_history_item(&h->items[i]);
    }

    memmove(h->items, h->items + drop, (size_t)(h->count - drop) * sizeof(*h->items));
    h->count -= drop;
}
