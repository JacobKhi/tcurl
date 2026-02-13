#include "test.h"

#include "core/auth.h"
#include "core/export.h"
#include "core/textbuf.h"

int test_export_auth(void) {
    RequestSnapshot req;
    memset(&req, 0, sizeof(req));
    req.method = HTTP_POST;
    req.url = "https://example.com/api?q=1";
    req.body_text = "{\"name\":\"a'b\"}";
    req.headers_text = "Content-Type: application/json\nX-Token: a'b";
    req.env_name = "dev";

    char *curl_cmd = export_as_curl(&req);
    TEST_ASSERT(curl_cmd != NULL);
    TEST_ASSERT(strstr(curl_cmd, "curl -X POST") != NULL);
    TEST_ASSERT(strstr(curl_cmd, "--data-binary") != NULL);
    free(curl_cmd);

    char *json = export_as_json(&req);
    TEST_ASSERT(json != NULL);
    TEST_ASSERT(strstr(json, "\"method\"") != NULL);
    TEST_ASSERT(strstr(json, "\"environment\"") != NULL);
    free(json);

    TextBuffer headers;
    tb_init(&headers);
    tb_set_from_string(&headers, "Authorization: old\nAccept: */*");
    TEST_ASSERT(auth_apply_bearer(&headers, "token-123") == 0);
    char *hdr = tb_to_string(&headers);
    TEST_ASSERT(hdr != NULL);
    TEST_ASSERT(strstr(hdr, "Authorization: Bearer token-123") != NULL);
    free(hdr);

    TEST_ASSERT(auth_apply_basic(&headers, "user", "pass") == 0);
    hdr = tb_to_string(&headers);
    TEST_ASSERT(hdr != NULL);
    TEST_ASSERT(strstr(hdr, "Authorization: Basic ") != NULL);
    free(hdr);
    tb_free(&headers);

    return 0;
}
