#pragma once

#include "core/history.h"

char *history_storage_default_path(void);

int history_storage_load(History *h, const char *path);
int history_storage_save(const History *h, const char *path);

int history_config_load_max_entries(const char *path, int fallback);
