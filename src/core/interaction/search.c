#include "core/interaction/search.h"
#include "core/storage/history.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/**
 * Case-insensitive substring search within a bounded string.
 * 
 * @param hay Haystack string (not necessarily null-terminated)
 * @param hay_n Length of haystack
 * @param needle Needle string (null-terminated)
 * @return 1 if needle found in hay, 0 otherwise
 */
static int contains_ci_n(const char *hay, size_t hay_n, const char *needle) {
    if (!needle || !needle[0]) return 1;
    size_t n = strlen(needle);
    if (n > hay_n) return 0;

    for (size_t i = 0; i + n <= hay_n; i++) {
        size_t j = 0;
        while (j < n) {
            unsigned char a = (unsigned char)hay[i + j];
            unsigned char b = (unsigned char)needle[j];
            if (tolower(a) != tolower(b)) break;
            j++;
        }
        if (j == n) return 1;
    }
    return 0;
}

/**
 * Get short string representation of HTTP method.
 */
static const char *method_short(int method) {
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DEL";
        default: return "?";
    }
}

/**
 * Check if a history item matches a search query.
 * 
 * Matches against "METHOD URL" format (case-insensitive).
 */
static int history_item_matches(const HistoryItem *it, const char *query) {
    char line[2048];
    snprintf(line, sizeof(line), "%s %s", method_short(it->method), it->url ? it->url : "");
    return contains_ci_n(line, strlen(line), query);
}

/**
 * Collect indices of all history items matching a query.
 * 
 * @param s Application state
 * @param query Search query
 * @param out_count Output: number of matches
 * @return Array of indices (caller must free), or NULL if no matches
 */
static int *collect_history_matches(const AppState *s, const char *query, int *out_count) {
    *out_count = 0;
    if (!s->history.history || s->history.history->count <= 0 || !query || !*query) return NULL;

    int *matches = malloc((size_t)s->history.history->count * sizeof(*matches));
    if (!matches) return NULL;

    int mcount = 0;
    for (int i = 0; i < s->history.history->count; i++) {
        if (history_item_matches(&s->history.history->items[i], query)) {
            matches[mcount++] = i;
        }
    }

    if (mcount == 0) {
        free(matches);
        return NULL;
    }

    *out_count = mcount;
    return matches;
}

/**
 * Collect line numbers of all lines in body matching a query.
 * 
 * @param body Response body text
 * @param query Search query
 * @param out_count Output: number of matches
 * @return Array of line numbers (caller must free), or NULL if no matches
 */
static int *collect_response_matches(const char *body, const char *query, int *out_count) {
    *out_count = 0;
    if (!body || !query || !*query) return NULL;

    int cap = 16;
    int count = 0;
    int *lines = malloc((size_t)cap * sizeof(*lines));
    if (!lines) return NULL;

    int line_no = 0;
    const char *p = body;
    while (1) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);

        if (contains_ci_n(p, len, query)) {
            if (count == cap) {
                int new_cap = cap * 2;
                int *n = realloc(lines, (size_t)new_cap * sizeof(*n));
                if (!n) {
                    free(lines);
                    return NULL;
                }
                lines = n;
                cap = new_cap;
            }
            lines[count++] = line_no;
        }

        if (!nl) break;
        p = nl + 1;
        line_no++;
    }

    if (count == 0) {
        free(lines);
        return NULL;
    }

    *out_count = count;
    return lines;
}

/**
 * Pick next/previous match with wrapping.
 * 
 * @param matches Array of match indices
 * @param count Number of matches
 * @param current Current index
 * @param dir Direction: positive for next, negative for previous
 * @return Next match index, or -1 if no matches
 */
static int pick_match_with_wrap(const int *matches, int count, int current, int dir) {
    if (!matches || count <= 0) return -1;

    if (current < 0) return dir >= 0 ? matches[0] : matches[count - 1];

    if (dir >= 0) {
        for (int i = 0; i < count; i++) {
            if (matches[i] > current) return matches[i];
        }
        return matches[0];
    }

    for (int i = count - 1; i >= 0; i--) {
        if (matches[i] < current) return matches[i];
    }
    return matches[count - 1];
}

/**
 * Apply search to history.
 */
static void apply_history_search(AppState *s, const char *query) {
    int mcount = 0;
    int *matches = collect_history_matches(s, query, &mcount);
    if (!matches) {
        s->search.not_found = 1;
        s->search.match_index = -1;
        return;
    }

    s->history.selected = matches[0];
    s->search.match_index = matches[0];
    s->search.not_found = 0;
    free(matches);
}

/**
 * Apply search to response body.
 */
static void apply_response_search(AppState *s, const char *query) {
    int mcount = 0;
    int *matches = collect_response_matches(s->response.response.body_view, query, &mcount);
    if (!matches) {
        s->search.not_found = 1;
        s->search.match_index = -1;
        return;
    }

    s->response.scroll = matches[0];
    s->search.match_index = matches[0];
    s->search.not_found = 0;
    free(matches);
}

/**
 * Step to next/previous match in history.
 */
static void step_history_search(AppState *s, int dir) {
    int mcount = 0;
    int *matches = collect_history_matches(s, s->search.query, &mcount);
    if (!matches) {
        s->search.not_found = 1;
        s->search.match_index = -1;
        return;
    }

    int next = pick_match_with_wrap(matches, mcount, s->search.match_index, dir);
    if (next >= 0) {
        s->history.selected = next;
        s->search.match_index = next;
        s->search.not_found = 0;
    } else {
        s->search.not_found = 1;
        s->search.match_index = -1;
    }
    free(matches);
}

/**
 * Step to next/previous match in response.
 */
static void step_response_search(AppState *s, int dir) {
    int mcount = 0;
    int *matches = collect_response_matches(s->response.response.body_view, s->search.query, &mcount);
    if (!matches) {
        s->search.not_found = 1;
        s->search.match_index = -1;
        return;
    }

    int next = pick_match_with_wrap(matches, mcount, s->search.match_index, dir);
    if (next >= 0) {
        s->response.scroll = next;
        s->search.match_index = next;
        s->search.not_found = 0;
    } else {
        s->search.not_found = 1;
        s->search.match_index = -1;
    }
    free(matches);
}

// Public API implementation

SearchTarget search_get_effective_target(const AppState *s) {
    if (s->search.target_override == SEARCH_TARGET_HISTORY) return SEARCH_TARGET_HISTORY;
    if (s->search.target_override == SEARCH_TARGET_RESPONSE) return SEARCH_TARGET_RESPONSE;
    return (s->ui.focused_panel == PANEL_HISTORY) ? SEARCH_TARGET_HISTORY : SEARCH_TARGET_RESPONSE;
}

void search_apply(AppState *s, const char *query) {
    if (!s) return;

    if (!query) query = "";
    strncpy(s->search.query, query, sizeof(s->search.query) - 1);
    s->search.query[sizeof(s->search.query) - 1] = '\0';
    s->search.match_index = -1;
    s->search.not_found = 0;

    if (s->search.query[0] == '\0') return;

    if (s->search.target == SEARCH_TARGET_HISTORY) {
        apply_history_search(s, s->search.query);
        return;
    }
    apply_response_search(s, s->search.query);
}

void search_step(AppState *s, int dir) {
    if (!s || s->search.query[0] == '\0') return;
    if (s->search.target == SEARCH_TARGET_HISTORY) {
        step_history_search(s, dir);
    } else {
        step_response_search(s, dir);
    }
}
