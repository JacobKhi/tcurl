#ifndef SEARCH_H
#define SEARCH_H

#include "state.h"

/**
 * Apply a search query to the current search target (history or response).
 * 
 * Updates the AppState with search results:
 * - Sets search_query
 * - Sets search_match_index to first match
 * - Sets search_not_found if no matches
 * - Updates history_selected or response_scroll to first match
 * 
 * @param s Application state (must not be NULL)
 * @param query Search query string (case-insensitive)
 */
void search_apply(AppState *s, const char *query);

/**
 * Step to next/previous search match.
 * 
 * @param s Application state (must not be NULL)
 * @param dir Direction: positive for next, negative for previous
 */
void search_step(AppState *s, int dir);

/**
 * Determine the effective search target based on current state.
 * 
 * Takes into account:
 * - search_target_override setting
 * - Currently focused panel
 * 
 * @param s Application state (must not be NULL)
 * @return SEARCH_TARGET_HISTORY or SEARCH_TARGET_RESPONSE
 */
SearchTarget search_get_effective_target(const AppState *s);

#endif // SEARCH_H
