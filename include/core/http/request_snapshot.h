#pragma once

#include "state.h"

typedef struct {
    HttpMethod method;
    char *url;
    char *body_text;
    char *headers_text;
    char *env_name;
} RequestSnapshot;

int request_snapshot_build(const AppState *s, RequestSnapshot *out);
int request_snapshot_build_locked(const AppState *s, RequestSnapshot *out);
void request_snapshot_free(RequestSnapshot *s);
