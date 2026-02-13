#pragma once

#include "core/history.h"

typedef struct {
    int loaded_ok;
    int skipped_invalid;
} HistoryLoadStats;

char *history_storage_default_path(void);

int history_storage_load(History *h, const char *path);
int history_storage_load_with_stats(History *h, const char *path, HistoryLoadStats *stats);
int history_storage_save(const History *h, const char *path);
int history_storage_append_last(const History *h, const char *path);

int history_config_load_max_entries(const char *path, int fallback);
