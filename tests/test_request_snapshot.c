#include "test.h"
#include "core/http/request_snapshot.h"
#include "state.h"
#include <string.h>

static void init_minimal_state(AppState *s) {
    memset(s, 0, sizeof(AppState));
    tb_init(&s->editor.body);
    tb_init(&s->editor.headers);
}

static void cleanup_state(AppState *s) {
    tb_free(&s->editor.body);
    tb_free(&s->editor.headers);
}

static int test_snapshot_null_checks(void) {
    AppState s;
    RequestSnapshot snap;
    
    TEST_ASSERT(request_snapshot_build(NULL, &snap) != 0);
    TEST_ASSERT(request_snapshot_build(&s, NULL) != 0);
    
    request_snapshot_free(NULL);
    return 0;
}

static int test_snapshot_basic(void) {
    AppState s;
    init_minimal_state(&s);
    
    strcpy(s.editor.url, "https://api.example.com/users");
    s.editor.method = HTTP_POST;
    
    tb_set_from_string(&s.editor.body, "{\"name\":\"test\"}");
    tb_set_from_string(&s.editor.headers, "Content-Type: application/json\nAuthorization: Bearer token");
    
    RequestSnapshot snap;
    int rc = request_snapshot_build(&s, &snap);
    
    TEST_ASSERT(rc == 0);
    TEST_ASSERT(snap.method == HTTP_POST);
    TEST_ASSERT(strcmp(snap.url, "https://api.example.com/users") == 0);
    TEST_ASSERT(strcmp(snap.body_text, "{\"name\":\"test\"}") == 0);
    TEST_ASSERT(strstr(snap.headers_text, "Content-Type") != NULL);
    
    request_snapshot_free(&snap);
    cleanup_state(&s);
    return 0;
}

static int test_snapshot_empty_fields(void) {
    AppState s;
    init_minimal_state(&s);
    
    s.editor.url[0] = '\0';
    s.editor.method = HTTP_GET;
    
    RequestSnapshot snap;
    int rc = request_snapshot_build(&s, &snap);
    
    TEST_ASSERT(rc == 0);
    TEST_ASSERT(snap.url != NULL);
    TEST_ASSERT(snap.url[0] == '\0');
    TEST_ASSERT(snap.body_text != NULL);
    TEST_ASSERT(snap.headers_text != NULL);
    
    request_snapshot_free(&snap);
    cleanup_state(&s);
    return 0;
}

static int test_snapshot_free_clears_memory(void) {
    AppState s;
    init_minimal_state(&s);
    
    strcpy(s.editor.url, "http://test.com");
    
    RequestSnapshot snap;
    request_snapshot_build(&s, &snap);
    
    request_snapshot_free(&snap);
    
    TEST_ASSERT(snap.url == NULL);
    TEST_ASSERT(snap.body_text == NULL);
    TEST_ASSERT(snap.headers_text == NULL);
    TEST_ASSERT(snap.env_name == NULL);
    
    cleanup_state(&s);
    return 0;
}

int test_request_snapshot(void) {
    int failed = 0;
    failed += test_snapshot_null_checks();
    failed += test_snapshot_basic();
    failed += test_snapshot_empty_fields();
    failed += test_snapshot_free_clears_memory();
    
    if (failed) {
        printf("test_request_snapshot: FAILED (%d tests)\n", failed);
        return 1;
    }
    printf("test_request_snapshot: OK\n");
    return 0;
}
