#pragma once
#include "state.h"

int http_request(const char *url, HttpMethod method, const char *body, HttpResponse *out);
