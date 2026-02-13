#include "test.h"

#include "core/history.h"
#include "core/history_storage.h"
#include "core/textbuf.h"
#include "state.h"

int test_history_storage(void) {
    const char *fixture = "tests/fixtures/history_corrupt.jsonl";
    const char *path = "/tmp/tcurl_history_runtime.jsonl";

    FILE *fin = fopen(fixture, "rb");
    TEST_ASSERT(fin != NULL);
    FILE *fout = fopen(path, "wb");
    TEST_ASSERT(fout != NULL);
    char buf[1024];
    size_t nread;
    while ((nread = fread(buf, 1, sizeof(buf), fin)) > 0) {
        TEST_ASSERT(fwrite(buf, 1, nread, fout) == nread);
    }
    fclose(fin);
    fclose(fout);

    History h;
    history_init(&h);
    HistoryLoadStats stats;
    TEST_ASSERT(history_storage_load_with_stats(&h, path, &stats) == 0);
    TEST_ASSERT(stats.loaded_ok == 2);
    TEST_ASSERT(stats.skipped_invalid == 1);
    TEST_ASSERT(h.count == 2);

    TextBuffer b;
    TextBuffer hd;
    tb_init(&b);
    tb_init(&hd);
    tb_set_from_string(&b, "{\"x\":1}");
    tb_set_from_string(&hd, "X-Test: 1");

    HttpResponse r;
    memset(&r, 0, sizeof(r));
    r.status = 202;
    r.body = strdup("resp");
    r.body_view = strdup("resp");
    r.elapsed_ms = 4.0;
    r.is_json = 0;
    history_push(&h, HTTP_POST, "https://c", &b, &hd, &r);

    TEST_ASSERT(history_storage_append_last(&h, path) == 0);
    TEST_ASSERT(history_storage_save(&h, "/tmp/tcurl_history_saved.jsonl") == 0);

    free(r.body);
    free(r.body_view);
    tb_free(&b);
    tb_free(&hd);
    history_free(&h);
    return 0;
}
