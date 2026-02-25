#include "core/http/http.h"
#include "core/utils/utils.h"
#include "core/config/constants.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

typedef struct {
    char *data;
    size_t size;
} Buffer;

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    Buffer *buf = userdata;

    char *newp = realloc(buf->data, buf->size + total + 1);
    if (!newp) return 0;

    buf->data = newp;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

static size_t header_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    Buffer *buf = userdata;

    char *newp = realloc(buf->data, buf->size + total + 1);
    if (!newp) return 0;

    buf->data = newp;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

static int starts_with_ci(const char *s, const char *p) {
    while (*p && *s) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*p))
            return 0;
        s++; p++;
    }
    return *p == '\0';
}

static struct curl_slist *
headers_from_textbuf(const TextBuffer *tb, int *has_content_type) {
    if (has_content_type) *has_content_type = 0;
    if (!tb) return NULL;

    struct curl_slist *list = NULL;

    for (int i = 0; i < tb->line_count; i++) {
        const char *line = tb->lines[i];
        if (!line || !*line) continue;

        char *tmp = strdup(line);
        if (!tmp) continue;

        char *p = str_trim_left(tmp);
        str_trim_right(p);

        char *colon = strchr(p, ':');
        if (!colon) { free(tmp); continue; }

        *colon = '\0';
        char *key = str_trim_left(p);
        str_trim_right(key);

        char *val = str_trim_left(colon + 1);
        str_trim_right(val);

        if (!*key) { free(tmp); continue; }

        if (has_content_type && starts_with_ci(key, "content-type"))
            *has_content_type = 1;

        char header[HTTP_HEADER_MAX];
        if (*val)
            snprintf(header, sizeof(header), "%s: %s", key, val);
        else
            snprintf(header, sizeof(header), "%s:", key);

        list = curl_slist_append(list, header);
        free(tmp);
    }

    return list;
}

int http_request(
    const char *url,
    HttpMethod method,
    const char *body,
    const TextBuffer *headers_tb,
    HttpResponse *out
) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    Buffer buf = {0};
    Buffer header_buf = {0};
    char errbuf[CURL_ERROR_SIZE];
    errbuf[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_buf);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    int has_ct = 0;
    struct curl_slist *headers =
        headers_from_textbuf(headers_tb, &has_ct);

    int sends_body = (method == HTTP_POST || method == HTTP_PUT);

    if (sends_body && !has_ct)
        headers = curl_slist_append(headers, "Content-Type: application/json");

    if (headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    const char *payload = body ? body : "";

    switch (method) {
        case HTTP_GET:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;

        case HTTP_POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(payload));
            break;

        case HTTP_PUT:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(payload));
            break;

        case HTTP_DELETE:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;

        case HTTP_PATCH:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(payload));
            break;

        case HTTP_HEAD:
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
            break;

        case HTTP_OPTIONS:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
            break;

        case HTTP_METHOD_COUNT:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;
    }

    CURLcode res = curl_easy_perform(curl);

    /* Capture detailed timing breakdown */
    double dns, tcp_conn, tls_conn, pre, ttfb, total;
    curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &dns);
    curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &tcp_conn);
    curl_easy_getinfo(curl, CURLINFO_APPCONNECT_TIME, &tls_conn);
    curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME, &pre);
    curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &ttfb);
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);

    /* Calculate timing deltas */
    out->timing.dns_ms = dns * 1000.0;
    out->timing.tcp_ms = (tcp_conn - dns) * 1000.0;
    out->timing.tls_ms = (tls_conn > 0) ? (tls_conn - tcp_conn) * 1000.0 : 0.0;
    out->timing.ttfb_ms = (ttfb > pre) ? (ttfb - pre) * 1000.0 : 0.0;
    out->timing.transfer_ms = (total > ttfb) ? (total - ttfb) * 1000.0 : 0.0;
    out->timing.total_ms = total * 1000.0;
    out->elapsed_ms = out->timing.total_ms;

    if (res != CURLE_OK) {
        out->status = 0;
        out->body = NULL;
        out->response_headers = header_buf.data;
        out->error = strdup(errbuf[0] ? errbuf : curl_easy_strerror(res));

        if (headers) curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free(buf.data);
        return -1;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out->status);
    out->body = buf.data;
    out->response_headers = header_buf.data;
    out->error = NULL;

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return 0;
}
