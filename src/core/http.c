#include "core/http.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

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

int http_get(const char *url, HttpResponse *out) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    Buffer buf = {0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        free(buf.data);
        return -1;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out->status);
    out->body = buf.data;

    curl_easy_cleanup(curl);
    return 0;
}
