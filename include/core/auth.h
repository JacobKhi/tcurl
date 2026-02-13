#pragma once

#include "core/textbuf.h"

int auth_apply_bearer(TextBuffer *headers, const char *token);
int auth_apply_basic(TextBuffer *headers, const char *user, const char *pass);
