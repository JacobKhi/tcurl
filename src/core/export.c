#include "core/export.h"
#include "core/cjson_compat.h"
#include "core/utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *method_name(HttpMethod m) {
    switch (m) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE";
        default: return "GET";
    }
}

static char *shell_quote_single(const char *in) {
    const char *s = in ? in : "";
    size_t n = 2;
    for (size_t i = 0; s[i]; i++) {
        if (s[i] == '\'') n += 6;
        else n += 1;
    }

    char *out = malloc(n + 1);
    if (!out) return NULL;

    size_t p = 0;
    out[p++] = '\'';
    for (size_t i = 0; s[i]; i++) {
        if (s[i] == '\'') {
            memcpy(out + p, "'\"'\"'", 6);
            p += 6;
        } else {
            out[p++] = s[i];
        }
    }
    out[p++] = '\'';
    out[p] = '\0';
    return out;
}

char *export_as_curl(const RequestSnapshot *req) {
    if (!req) return NULL;

    char *url_q = shell_quote_single(req->url);
    if (!url_q) return NULL;

    char *buf = NULL;
    size_t len = 0, cap = 0;
    if (!str_appendf(&buf, &len, &cap, "curl -X %s %s", method_name(req->method), url_q)) {
        free(url_q);
        free(buf);
        return NULL;
    }
    free(url_q);

    char *headers = strdup(req->headers_text ? req->headers_text : "");
    if (!headers) {
        free(buf);
        return NULL;
    }

    char *save = NULL;
    for (char *line = strtok_r(headers, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        while (*line == ' ' || *line == '\t') line++;
        if (!*line) continue;
        char *h_q = shell_quote_single(line);
        if (!h_q) {
            free(headers);
            free(buf);
            return NULL;
        }
        if (!str_appendf(&buf, &len, &cap, " -H %s", h_q)) {
            free(h_q);
            free(headers);
            free(buf);
            return NULL;
        }
        free(h_q);
    }
    free(headers);

    if (req->body_text && req->body_text[0] &&
        (req->method == HTTP_POST || req->method == HTTP_PUT)) {
        char *body_q = shell_quote_single(req->body_text);
        if (!body_q) {
            free(buf);
            return NULL;
        }
        if (!str_appendf(&buf, &len, &cap, " --data-binary %s", body_q)) {
            free(body_q);
            free(buf);
            return NULL;
        }
        free(body_q);
    }

    if (!str_appendf(&buf, &len, &cap, "\n")) {
        free(buf);
        return NULL;
    }
    return buf;
}

char *export_as_json(const RequestSnapshot *req) {
    if (!req) return NULL;

    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, "method", method_name(req->method));
    cJSON_AddStringToObject(root, "url", req->url ? req->url : "");
    cJSON_AddStringToObject(root, "body", req->body_text ? req->body_text : "");
    cJSON_AddStringToObject(root, "headers", req->headers_text ? req->headers_text : "");
    cJSON_AddStringToObject(root, "environment", req->env_name ? req->env_name : "");

    char *out = cJSON_Print(root);
    cJSON_Delete(root);
    return out;
}
