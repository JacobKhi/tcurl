#include "core/format.h"
#include "core/cjson_compat.h"
#include <stdlib.h>

char *json_pretty_print(const char *input) {
    if (!input) return NULL;

    cJSON *root = cJSON_Parse(input);
    if (!root) return NULL;

    char *pretty = cJSON_Print(root);
    cJSON_Delete(root);

    return pretty;
}
