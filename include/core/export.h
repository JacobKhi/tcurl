#pragma once

#include "core/request_snapshot.h"

char *export_as_curl(const RequestSnapshot *req);
char *export_as_json(const RequestSnapshot *req);
