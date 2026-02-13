#include "core/http.h"
#include <ctype.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t size;
} Buffer;

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    Buffer *buf = (Buffer *)userdata;

    char *newp = (char *)realloc(buf->data, buf->size + total + 1);
    if (!newp) return 0;

    buf->data = newp;
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

static void rtrim(char *s) {
    int n = (int)strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        s[n - 1] = '\0';
        n--;
    }
}

static char *ltrim_ptr(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

static int starts_with_ci(const char *s, const char *prefix) {
    while (*prefix && *s) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return 0;
        s++;
        prefix++;
    }
    return *prefix == '\0';
}

static struct curl_slist *headers_from_textbuf(const TextBuffer *tb, int *has_content_type) {
    if (has_content_type) *has_content_type = 0;
    if (!tb) return NULL;

    struct curl_slist *list = NULL;

    for (int i = 0; i < tb->line_count; i++) {
        const char *line = tb->lines[i];
        if (!line || line[0] == '\0') continue;

        char *tmp = strdup(line);
        if (!tmp) continue;

        char *p = ltrim_ptr(tmp);
        rtrim(p);
        if (*p == '\0') { free(tmp); continue; }

        char *colon = strchr(p, ':');
        if (!colon) { free(tmp); continue; }

        *colon = '\0';
        char *key = ltrim_ptr(p);
        rtrim(key);

        char *value = ltrim_ptr(colon + 1);
        rtrim(value);

        if (*key == '\0') { free(tmp); continue; }

        if (has_content_type && starts_with_ci(key, "content-type")) {
            *has_content_type = 1;
        }

        size_t need = strlen(key) + 2 + strlen(value) + 1;
        char *hv = (char *)malloc(need);
        if (!hv) { free(tmp); continue; }

        if (*value) snprintf(hv, need, "%s: %s", key, value);
        else snprintf(hv, need, "%s:", key);

        list = curl_slist_append(list, hv);

        free(hv);
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
    if (!url || !out) return -1;

    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    Buffer buf = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    const char *payload = body ? body : "";

    int has_content_type = 0;
    struct curl_slist *headers = headers_from_textbuf(headers_tb, &has_content_type);

    int sending_body = (method == HTTP_POST || method == HTTP_PUT);
    if (sending_body && !has_content_type) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }

    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

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

        default:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;
    }

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        out->status = 0;
        free(buf.data);
        buf.data = NULL;

        if (headers) curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out->status);
    out->body = buf.data;

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return 0;
}
