#pragma once

#include "core/http/request_snapshot.h"

char *export_as_curl(const RequestSnapshot *req);
char *export_as_json(const RequestSnapshot *req);
