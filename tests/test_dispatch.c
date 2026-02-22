#include "test.h"
#include "state.h"
#include "orchestration/dispatch.h"
#include "core/interaction/actions.h"
#include "core/storage/history.h"
#include <string.h>

static void init_minimal_state(AppState *s) {
    memset(s, 0, sizeof(*s));
    s->running = 1;
    s->ui.mode = MODE_NORMAL;
    s->ui.focused_panel = PANEL_EDITOR;
    s->ui.language = UI_LANG_EN;
    s->editor.method = HTTP_GET;
    s->editor.active_field = EDIT_FIELD_URL;
    s->history.selected = 0;
    tb_init(&s->editor.body);
    tb_init(&s->editor.headers);
}

static void cleanup_state(AppState *s) {
    tb_free(&s->editor.body);
    tb_free(&s->editor.headers);
}

static int test_dispatch_quit(void) {
    AppState s;
    init_minimal_state(&s);
    
    TEST_ASSERT(s.running == 1);
    dispatch_action(&s, ACT_QUIT);
    TEST_ASSERT(s.running == 0);
    
    cleanup_state(&s);
    return 0;
}

static int test_dispatch_mode_changes(void) {
    AppState s;
    init_minimal_state(&s);
    
    TEST_ASSERT(s.ui.mode == MODE_NORMAL);
    
    dispatch_action(&s, ACT_ENTER_INSERT);
    TEST_ASSERT(s.ui.mode == MODE_INSERT);
    
    dispatch_action(&s, ACT_ENTER_NORMAL);
    TEST_ASSERT(s.ui.mode == MODE_NORMAL);
    
    dispatch_action(&s, ACT_ENTER_COMMAND);
    TEST_ASSERT(s.ui.mode == MODE_COMMAND);
    TEST_ASSERT(s.prompt.kind == PROMPT_COMMAND);
    
    dispatch_action(&s, ACT_ENTER_SEARCH);
    TEST_ASSERT(s.ui.mode == MODE_SEARCH);
    TEST_ASSERT(s.prompt.kind == PROMPT_SEARCH);
    
    cleanup_state(&s);
    return 0;
}

static int test_dispatch_panel_navigation(void) {
    AppState s;
    init_minimal_state(&s);
    
    s.ui.focused_panel = PANEL_HISTORY;
    TEST_ASSERT(s.ui.focused_panel == PANEL_HISTORY);
    
    dispatch_action(&s, ACT_FOCUS_RIGHT);
    TEST_ASSERT(s.ui.focused_panel == PANEL_EDITOR);
    
    dispatch_action(&s, ACT_FOCUS_RIGHT);
    TEST_ASSERT(s.ui.focused_panel == PANEL_RESPONSE);
    
    dispatch_action(&s, ACT_FOCUS_LEFT);
    TEST_ASSERT(s.ui.focused_panel == PANEL_EDITOR);
    
    dispatch_action(&s, ACT_FOCUS_LEFT);
    TEST_ASSERT(s.ui.focused_panel == PANEL_HISTORY);
    
    cleanup_state(&s);
    return 0;
}

static int test_dispatch_method_cycle(void) {
    AppState s;
    init_minimal_state(&s);
    
    s.editor.method = HTTP_GET;
    TEST_ASSERT(s.editor.method == HTTP_GET);
    
    dispatch_action(&s, ACT_CYCLE_METHOD);
    TEST_ASSERT(s.editor.method == HTTP_POST);
    
    dispatch_action(&s, ACT_CYCLE_METHOD);
    TEST_ASSERT(s.editor.method == HTTP_PUT);
    
    dispatch_action(&s, ACT_CYCLE_METHOD);
    TEST_ASSERT(s.editor.method == HTTP_DELETE);
    
    cleanup_state(&s);
    return 0;
}

static int test_dispatch_editor_field_toggle(void) {
    AppState s;
    init_minimal_state(&s);
    
    s.ui.focused_panel = PANEL_EDITOR;
    s.editor.active_field = EDIT_FIELD_URL;
    
    dispatch_action(&s, ACT_TOGGLE_EDITOR_FIELD);
    TEST_ASSERT(s.editor.active_field == EDIT_FIELD_BODY);
    
    dispatch_action(&s, ACT_TOGGLE_EDITOR_FIELD);
    TEST_ASSERT(s.editor.active_field == EDIT_FIELD_HEADERS);
    
    dispatch_action(&s, ACT_TOGGLE_EDITOR_FIELD);
    TEST_ASSERT(s.editor.active_field == EDIT_FIELD_URL);
    
    cleanup_state(&s);
    return 0;
}

static int test_dispatch_history_navigation(void) {
    AppState s;
    init_minimal_state(&s);
    
    History h;
    history_init(&h);
    s.history.history = &h;
    s.history.selected = 0;
    s.ui.focused_panel = PANEL_HISTORY;
    
    TextBuffer tb1, tb2;
    tb_init(&tb1);
    tb_init(&tb2);
    
    HttpResponse resp1 = {0};
    resp1.status = 200;
    history_push(&h, HTTP_GET, "http://test1.com", &tb1, &tb2, &resp1);
    
    HttpResponse resp2 = {0};
    resp2.status = 200;
    history_push(&h, HTTP_POST, "http://test2.com", &tb1, &tb2, &resp2);
    
    TEST_ASSERT(s.history.selected == 0);
    
    dispatch_action(&s, ACT_MOVE_DOWN);
    TEST_ASSERT(s.history.selected == 1);
    
    dispatch_action(&s, ACT_MOVE_DOWN);
    TEST_ASSERT(s.history.selected == 1);
    
    dispatch_action(&s, ACT_MOVE_UP);
    TEST_ASSERT(s.history.selected == 0);
    
    dispatch_action(&s, ACT_MOVE_UP);
    TEST_ASSERT(s.history.selected == 0);
    
    tb_free(&tb1);
    tb_free(&tb2);
    history_free(&h);
    cleanup_state(&s);
    return 0;
}

static int test_dispatch_response_scroll(void) {
    AppState s;
    init_minimal_state(&s);
    
    s.ui.focused_panel = PANEL_RESPONSE;
    s.response.scroll = 0;
    
    dispatch_action(&s, ACT_MOVE_DOWN);
    TEST_ASSERT(s.response.scroll == 1);
    
    dispatch_action(&s, ACT_MOVE_DOWN);
    TEST_ASSERT(s.response.scroll == 2);
    
    dispatch_action(&s, ACT_MOVE_UP);
    TEST_ASSERT(s.response.scroll == 1);
    
    dispatch_action(&s, ACT_MOVE_UP);
    TEST_ASSERT(s.response.scroll == 0);
    
    dispatch_action(&s, ACT_MOVE_UP);
    TEST_ASSERT(s.response.scroll == 0);
    
    cleanup_state(&s);
    return 0;
}

int test_dispatch(void) {
    int failed = 0;
    failed += test_dispatch_quit();
    failed += test_dispatch_mode_changes();
    failed += test_dispatch_panel_navigation();
    failed += test_dispatch_method_cycle();
    failed += test_dispatch_editor_field_toggle();
    failed += test_dispatch_history_navigation();
    failed += test_dispatch_response_scroll();
    
    if (failed) {
        printf("test_dispatch: FAILED (%d tests)\n", failed);
        return 1;
    }
    printf("test_dispatch: OK\n");
    return 0;
}
