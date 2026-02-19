#include "core/i18n.h"
#include "core/utils.h"

#include <ctype.h>
#include <stddef.h>

static int starts_with_pt(const char *s) {
    if (!s || !s[0] || !s[1]) return 0;
    return tolower((unsigned char)s[0]) == 'p' &&
           tolower((unsigned char)s[1]) == 't';
}

static const char *const EN[I18N_COUNT] = {
    [I18N_MODE_NORMAL] = "NORMAL",
    [I18N_MODE_INSERT] = "INSERT",
    [I18N_MODE_COMMAND] = "COMMAND",
    [I18N_MODE_SEARCH] = "SEARCH",
    [I18N_MODE_UNKNOWN] = "UNKNOWN",

    [I18N_MODE_NORMAL_LC] = "normal",
    [I18N_MODE_INSERT_LC] = "insert",
    [I18N_MODE_COMMAND_LC] = "command",
    [I18N_MODE_SEARCH_LC] = "search",
    [I18N_MODE_UNKNOWN_LC] = "unknown",

    [I18N_PANEL_HISTORY] = "HISTORY",
    [I18N_PANEL_EDITOR] = "EDITOR",
    [I18N_PANEL_RESPONSE] = "RESPONSE",
    [I18N_PANEL_UNKNOWN] = "UNKNOWN",

    [I18N_SENDING_REQUEST] = "Sending request...",
    [I18N_REQUEST_FAILED] = "Request failed:",
    [I18N_NO_RESPONSE_YET] = "No response yet",
    [I18N_RESPONSE_META_FMT] = "Status: %ld | Time: %.0f ms | Size: %.1f KB%s | scroll:%d",
    [I18N_RESPONSE_META_JSON] = " | JSON",
    [I18N_URL_LABEL] = "URL:",
    [I18N_BODY_LABEL] = "BODY",
    [I18N_HEADERS_LABEL] = "HEADERS",
    [I18N_TOPBAR_FMT] = " tcurl | %s | layout=%s ",
    [I18N_STATUS_NOT_FOUND_FMT] = " %s | focus=%s | env=%s | history_selected=%d | load_skipped=%d | not found: %s ",
    [I18N_STATUS_DEFAULT_FMT] = " %s | focus=%s | env=%s | history_selected=%d | load_skipped=%d | save_err=%d ",
    [I18N_ENV_NONE] = "none",
    [I18N_HINT_FOOTER] = ":h help  :q quit  Move: h/j/k/l or arrows",
    [I18N_WIN_HISTORY] = " History ",
    [I18N_WIN_EDITOR_TABS] = " Editor [TABS] ",
    [I18N_WIN_EDITOR_URL] = " Editor [URL] ",
    [I18N_WIN_EDITOR_BODY] = " Editor [BODY] ",
    [I18N_WIN_EDITOR_HEADERS] = " Editor [HEADERS] ",
    [I18N_WIN_RESPONSE] = " Response ",

    [I18N_UNKNOWN_ERROR] = "Unknown error",
    [I18N_USAGE_THEME_NAME_SAVE] = "Usage: :theme <name> [-s|--save]",
    [I18N_USAGE_THEME_NAME_SAVE_OR_LIST] = "Usage: :theme <name> [-s|--save] | :theme list",
    [I18N_UNKNOWN_THEME_PRESET] = "Unknown theme preset. Use :theme list",
    [I18N_THEME_APPLIED_SAVED_FMT] = "Theme '%s' applied and saved to %s",
    [I18N_THEME_APPLIED_SAVE_FAILED_FMT] = "Theme '%s' applied for this session, but failed to save %s",
    [I18N_THEME_APPLIED_SESSION_FMT] = "Theme '%s' applied for this session",
    [I18N_OOM_LISTING_THEMES] = "Out of memory listing themes",
    [I18N_HISTORY_NOT_INITIALIZED] = "History is not initialized",
    [I18N_CANNOT_CLEAR_HISTORY_IN_FLIGHT] = "Cannot clear history while request is in flight",
    [I18N_HISTORY_CLEARED] = "History cleared",
    [I18N_HISTORY_CLEARED_SAVE_FAILED] = "History cleared in memory, but failed to persist storage",
    [I18N_USAGE_EXPORT] = "Usage: :export curl|json",
    [I18N_OOM_EXPORT_SNAPSHOT] = "Out of memory creating export snapshot",
    [I18N_UNKNOWN_EXPORT_FORMAT] = "Unknown export format. Use curl or json",
    [I18N_EXPORT_FAILED] = "Export failed",
    [I18N_USAGE_AUTH] = "Usage: :auth bearer <token> | :auth basic <user>:<pass>",
    [I18N_USAGE_AUTH_BASIC] = "Usage: :auth basic <user>:<pass>",
    [I18N_OOM_APPLY_AUTH] = "Out of memory applying auth",
    [I18N_UNKNOWN_AUTH_KIND] = "Unknown auth kind. Use bearer or basic",
    [I18N_AUTH_UPDATED] = "Authorization header updated",
    [I18N_AUTH_UPDATE_FAILED] = "Failed to update Authorization header",
    [I18N_USAGE_FIND] = "Usage: :find <term>",
    [I18N_SETTINGS_FMT] = "Settings:\n- search_target=%s\n- history_max_entries=%d",
    [I18N_USAGE_SET_SEARCH_TARGET] = "Usage: :set search_target auto|history|response",
    [I18N_SEARCH_TARGET_UPDATED] = "search_target updated",
    [I18N_USAGE_SET_MAX_ENTRIES] = "Usage: :set max_entries <int>",
    [I18N_MAX_ENTRIES_UPDATED_SESSION] = "max_entries updated (session only)",
    [I18N_UNKNOWN_SETTING] = "Unknown setting. Use search_target or max_entries",
    [I18N_USAGE_LANG] = "Usage: :lang auto|en|pt | :lang list",
    [I18N_USAGE_LANG_LIST] = "Usage: :lang list",
    [I18N_UNKNOWN_LANGUAGE] = "Unknown language. Use auto, en, or pt",
    [I18N_LANGUAGE_UPDATED_FMT] = "Language set to '%s' for this session",
    [I18N_USAGE_LAYOUT] = "Usage: :layout classic|quad|focus_editor | :layout list",
    [I18N_USAGE_LAYOUT_LIST] = "Usage: :layout list",
    [I18N_UNKNOWN_LAYOUT] = "Unknown layout. Use classic, quad, or focus_editor",
    [I18N_LAYOUT_UPDATED_FMT] = "Layout set to '%s' for this session",
    [I18N_NO_KEYMAP_LOADED] = "No keymap loaded.",
    [I18N_HELP_HEADER_BASIC] = "BASIC COMMANDS:\n",
    [I18N_HELP_HEADER_LANG] = "\nLANGUAGE:\n",
    [I18N_HELP_HEADER_LAYOUT] = "\nLAYOUT:\n",
    [I18N_HELP_HEADER_THEME] = "\nTHEME MANAGEMENT:\n",
    [I18N_HELP_HEADER_EXPORT] = "\nEXPORT:\n",
    [I18N_HELP_HEADER_AUTH] = "\nAUTHENTICATION:\n",
    [I18N_HELP_HEADER_SEARCH] = "\nSEARCH:\n",
    [I18N_HELP_HEADER_HISTORY] = "\nHISTORY:\n",
    [I18N_HELP_HEADER_SETTINGS] = "\nSETTINGS:\n",
    [I18N_HELP_CMD_QUIT] = "  :q | :quit              Quit application\n",
    [I18N_HELP_CMD_HELP] = "  :h | :help              Show this help\n",
    [I18N_HELP_CMD_LANG_LIST] = "  :lang list              List available languages\n",
    [I18N_HELP_CMD_LANG_SET] = "  :lang <auto|en|pt>      Set UI language for current session\n",
    [I18N_HELP_CMD_LAYOUT_LIST] = "  :layout list            List available layouts\n",
    [I18N_HELP_CMD_LAYOUT_SET] = "  :layout <name>          Set layout profile for current session\n",
    [I18N_HELP_CMD_THEME_LIST] = "  :theme list             List available theme presets\n",
    [I18N_HELP_CMD_THEME_APPLY] = "  :theme <name>           Apply theme preset for current session\n",
    [I18N_HELP_CMD_THEME_SAVE] = "  :theme <name> -s|--save Apply and persist active preset\n",
    [I18N_HELP_CMD_EXPORT] = "  :export curl|json       Export current request\n",
    [I18N_HELP_CMD_AUTH_BEARER] = "  :auth bearer <token>    Set Authorization bearer header\n",
    [I18N_HELP_CMD_AUTH_BASIC] = "  :auth basic <user>:<pass>  Set Authorization basic header\n",
    [I18N_HELP_CMD_FIND] = "  :find <term>            Run contextual search immediately\n",
    [I18N_HELP_CMD_SET] = "  :set [key] [value]      Update runtime settings\n                          Keys: search_target, max_entries\n",
    [I18N_HELP_CMD_CLEAR] = "  :clear! | :ch!          Clear history (memory + storage)\n",
    [I18N_HELP_NAV_HEADER] = "\nNAVIGATION:\n",
    [I18N_HELP_NAV_LINE] = "  Arrow keys mirror h/l/k/j in normal mode\n",
    [I18N_HELP_KEYS_HEADER] = "\nKEY BINDINGS:\n",
    [I18N_HELP_NO_DESC] = "No description available",
    [I18N_OOM_BUILD_HELP] = "Out of memory building help text",
    [I18N_USAGE_THEME_LIST] = "Usage: :theme list",
    [I18N_INVALID_THEME_FLAG] = "Invalid flag. Use -s or --save",
    [I18N_USAGE_SET_KEY_VALUE] = "Usage: :set <key> <value>",
    [I18N_UNKNOWN_COMMAND_FMT] = "Unknown command: %s",

    [I18N_ACT_QUIT_DESC] = "Quit application",
    [I18N_ACT_MOVE_DOWN_DESC] = "Move down in focused panel",
    [I18N_ACT_MOVE_UP_DESC] = "Move up in focused panel",
    [I18N_ACT_FOCUS_LEFT_DESC] = "Focus panel to the left",
    [I18N_ACT_FOCUS_RIGHT_DESC] = "Focus panel to the right",
    [I18N_ACT_ENTER_INSERT_DESC] = "Enter insert mode",
    [I18N_ACT_ENTER_NORMAL_DESC] = "Return to normal mode",
    [I18N_ACT_ENTER_COMMAND_DESC] = "Open command prompt",
    [I18N_ACT_ENTER_SEARCH_DESC] = "Open search prompt",
    [I18N_ACT_SEND_REQUEST_DESC] = "Send current request",
    [I18N_ACT_TOGGLE_EDITOR_FIELD_DESC] = "Cycle editor field",
    [I18N_ACT_CYCLE_METHOD_DESC] = "Cycle HTTP method",
    [I18N_ACT_CYCLE_ENV_DESC] = "Cycle active environment",
    [I18N_ACT_HISTORY_LOAD_DESC] = "Load selected history item",
    [I18N_ACT_HISTORY_REPLAY_DESC] = "Replay selected history request",
    [I18N_ACT_SEARCH_NEXT_DESC] = "Go to next search match",
    [I18N_ACT_SEARCH_PREV_DESC] = "Go to previous search match",
};

static const char *const PT[I18N_COUNT] = {
    [I18N_MODE_NORMAL] = "NORMAL",
    [I18N_MODE_INSERT] = "INSERIR",
    [I18N_MODE_COMMAND] = "COMANDO",
    [I18N_MODE_SEARCH] = "BUSCA",
    [I18N_MODE_UNKNOWN] = "DESCONHECIDO",

    [I18N_MODE_NORMAL_LC] = "normal",
    [I18N_MODE_INSERT_LC] = "inserir",
    [I18N_MODE_COMMAND_LC] = "comando",
    [I18N_MODE_SEARCH_LC] = "busca",
    [I18N_MODE_UNKNOWN_LC] = "desconhecido",

    [I18N_PANEL_HISTORY] = "HISTÓRICO",
    [I18N_PANEL_EDITOR] = "EDITOR",
    [I18N_PANEL_RESPONSE] = "RESPOSTA",
    [I18N_PANEL_UNKNOWN] = "DESCONHECIDO",

    [I18N_SENDING_REQUEST] = "Enviando requisição...",
    [I18N_REQUEST_FAILED] = "Requisição falhou:",
    [I18N_NO_RESPONSE_YET] = "Sem resposta ainda",
    [I18N_RESPONSE_META_FMT] = "Status: %ld | Tempo: %.0f ms | Tamanho: %.1f KB%s | rolagem:%d",
    [I18N_RESPONSE_META_JSON] = " | JSON",
    [I18N_URL_LABEL] = "URL:",
    [I18N_BODY_LABEL] = "CORPO",
    [I18N_HEADERS_LABEL] = "HEADERS",
    [I18N_TOPBAR_FMT] = " tcurl | %s | layout=%s ",
    [I18N_STATUS_NOT_FOUND_FMT] = " %s | foco=%s | env=%s | histórico_sel=%d | load_skipped=%d | não encontrado: %s ",
    [I18N_STATUS_DEFAULT_FMT] = " %s | foco=%s | env=%s | histórico_sel=%d | load_skipped=%d | erro_save=%d ",
    [I18N_ENV_NONE] = "nenhum",
    [I18N_HINT_FOOTER] = ":h ajuda  :q sair  Mover: h/j/k/l ou setas",
    [I18N_WIN_HISTORY] = " Histórico ",
    [I18N_WIN_EDITOR_TABS] = " Editor [ABAS] ",
    [I18N_WIN_EDITOR_URL] = " Editor [URL] ",
    [I18N_WIN_EDITOR_BODY] = " Editor [CORPO] ",
    [I18N_WIN_EDITOR_HEADERS] = " Editor [HEADERS] ",
    [I18N_WIN_RESPONSE] = " Resposta ",

    [I18N_UNKNOWN_ERROR] = "Erro desconhecido",
    [I18N_USAGE_THEME_NAME_SAVE] = "Uso: :theme <name> [-s|--save]",
    [I18N_USAGE_THEME_NAME_SAVE_OR_LIST] = "Uso: :theme <name> [-s|--save] | :theme list",
    [I18N_UNKNOWN_THEME_PRESET] = "Preset de tema desconhecido. Use :theme list",
    [I18N_THEME_APPLIED_SAVED_FMT] = "Tema '%s' aplicado e salvo em %s",
    [I18N_THEME_APPLIED_SAVE_FAILED_FMT] = "Tema '%s' aplicado nesta sessão, mas falhou ao salvar %s",
    [I18N_THEME_APPLIED_SESSION_FMT] = "Tema '%s' aplicado nesta sessão",
    [I18N_OOM_LISTING_THEMES] = "Memória insuficiente ao listar temas",
    [I18N_HISTORY_NOT_INITIALIZED] = "Histórico não inicializado",
    [I18N_CANNOT_CLEAR_HISTORY_IN_FLIGHT] = "Não é possível limpar histórico com requisição em andamento",
    [I18N_HISTORY_CLEARED] = "Histórico limpo",
    [I18N_HISTORY_CLEARED_SAVE_FAILED] = "Histórico limpo em memória, mas falhou ao persistir armazenamento",
    [I18N_USAGE_EXPORT] = "Uso: :export curl|json",
    [I18N_OOM_EXPORT_SNAPSHOT] = "Memória insuficiente ao criar snapshot para export",
    [I18N_UNKNOWN_EXPORT_FORMAT] = "Formato de export desconhecido. Use curl ou json",
    [I18N_EXPORT_FAILED] = "Export falhou",
    [I18N_USAGE_AUTH] = "Uso: :auth bearer <token> | :auth basic <user>:<pass>",
    [I18N_USAGE_AUTH_BASIC] = "Uso: :auth basic <user>:<pass>",
    [I18N_OOM_APPLY_AUTH] = "Memória insuficiente ao aplicar auth",
    [I18N_UNKNOWN_AUTH_KIND] = "Tipo de auth desconhecido. Use bearer ou basic",
    [I18N_AUTH_UPDATED] = "Cabeçalho Authorization atualizado",
    [I18N_AUTH_UPDATE_FAILED] = "Falha ao atualizar cabeçalho Authorization",
    [I18N_USAGE_FIND] = "Uso: :find <term>",
    [I18N_SETTINGS_FMT] = "Configurações:\n- search_target=%s\n- history_max_entries=%d",
    [I18N_USAGE_SET_SEARCH_TARGET] = "Uso: :set search_target auto|history|response",
    [I18N_SEARCH_TARGET_UPDATED] = "search_target atualizado",
    [I18N_USAGE_SET_MAX_ENTRIES] = "Uso: :set max_entries <int>",
    [I18N_MAX_ENTRIES_UPDATED_SESSION] = "max_entries atualizado (apenas sessão)",
    [I18N_UNKNOWN_SETTING] = "Configuração desconhecida. Use search_target ou max_entries",
    [I18N_USAGE_LANG] = "Uso: :lang auto|en|pt | :lang list",
    [I18N_USAGE_LANG_LIST] = "Uso: :lang list",
    [I18N_UNKNOWN_LANGUAGE] = "Linguagem desconhecida. Use auto, en ou pt",
    [I18N_LANGUAGE_UPDATED_FMT] = "Linguagem definida como '%s' para esta sessao",
    [I18N_USAGE_LAYOUT] = "Uso: :layout classic|quad|focus_editor | :layout list",
    [I18N_USAGE_LAYOUT_LIST] = "Uso: :layout list",
    [I18N_UNKNOWN_LAYOUT] = "Layout desconhecido. Use classic, quad ou focus_editor",
    [I18N_LAYOUT_UPDATED_FMT] = "Layout definido como '%s' para esta sessao",
    [I18N_NO_KEYMAP_LOADED] = "Nenhum keymap carregado.",
    [I18N_HELP_HEADER_BASIC] = "COMANDOS BASICOS:\n",
    [I18N_HELP_HEADER_LANG] = "\nLINGUAGEM:\n",
    [I18N_HELP_HEADER_LAYOUT] = "\nLAYOUT:\n",
    [I18N_HELP_HEADER_THEME] = "\nGERENCIAMENTO DE TEMAS:\n",
    [I18N_HELP_HEADER_EXPORT] = "\nEXPORTACAO:\n",
    [I18N_HELP_HEADER_AUTH] = "\nAUTENTICACAO:\n",
    [I18N_HELP_HEADER_SEARCH] = "\nBUSCA:\n",
    [I18N_HELP_HEADER_HISTORY] = "\nHISTORICO:\n",
    [I18N_HELP_HEADER_SETTINGS] = "\nCONFIGURACOES:\n",
    [I18N_HELP_CMD_QUIT] = "  :q | :quit              Sair da aplicacao\n",
    [I18N_HELP_CMD_HELP] = "  :h | :help              Mostrar esta ajuda\n",
    [I18N_HELP_CMD_LANG_LIST] = "  :lang list              Listar linguagens disponiveis\n",
    [I18N_HELP_CMD_LANG_SET] = "  :lang <auto|en|pt>      Definir linguagem da UI para sessao atual\n",
    [I18N_HELP_CMD_LAYOUT_LIST] = "  :layout list            Listar layouts disponiveis\n",
    [I18N_HELP_CMD_LAYOUT_SET] = "  :layout <name>          Definir perfil de layout para sessao atual\n",
    [I18N_HELP_CMD_THEME_LIST] = "  :theme list             Listar presets de tema disponiveis\n",
    [I18N_HELP_CMD_THEME_APPLY] = "  :theme <name>           Aplicar preset de tema na sessao atual\n",
    [I18N_HELP_CMD_THEME_SAVE] = "  :theme <name> -s|--save Aplicar e persistir preset ativo\n",
    [I18N_HELP_CMD_EXPORT] = "  :export curl|json       Exportar requisicao atual\n",
    [I18N_HELP_CMD_AUTH_BEARER] = "  :auth bearer <token>    Definir cabecalho Authorization bearer\n",
    [I18N_HELP_CMD_AUTH_BASIC] = "  :auth basic <user>:<pass>  Definir cabecalho Authorization basic\n",
    [I18N_HELP_CMD_FIND] = "  :find <term>            Executar busca contextual imediatamente\n",
    [I18N_HELP_CMD_SET] = "  :set [chave] [valor]    Atualizar configuracoes de runtime\n                          Chaves: search_target, max_entries\n",
    [I18N_HELP_CMD_CLEAR] = "  :clear! | :ch!          Limpar historico (memoria + armazenamento)\n",
    [I18N_HELP_NAV_HEADER] = "\nNAVEGACAO:\n",
    [I18N_HELP_NAV_LINE] = "  Setas espelham h/l/k/j no modo normal\n",
    [I18N_HELP_KEYS_HEADER] = "\nATALHOS DE TECLADO:\n",
    [I18N_HELP_NO_DESC] = "Sem descrição disponível",
    [I18N_OOM_BUILD_HELP] = "Memória insuficiente ao montar texto de ajuda",
    [I18N_USAGE_THEME_LIST] = "Uso: :theme list",
    [I18N_INVALID_THEME_FLAG] = "Flag inválida. Use -s ou --save",
    [I18N_USAGE_SET_KEY_VALUE] = "Uso: :set <key> <value>",
    [I18N_UNKNOWN_COMMAND_FMT] = "Comando desconhecido: %s",

    [I18N_ACT_QUIT_DESC] = "Sair da aplicação",
    [I18N_ACT_MOVE_DOWN_DESC] = "Mover para baixo no painel focado",
    [I18N_ACT_MOVE_UP_DESC] = "Mover para cima no painel focado",
    [I18N_ACT_FOCUS_LEFT_DESC] = "Focar painel à esquerda",
    [I18N_ACT_FOCUS_RIGHT_DESC] = "Focar painel à direita",
    [I18N_ACT_ENTER_INSERT_DESC] = "Entrar no modo inserir",
    [I18N_ACT_ENTER_NORMAL_DESC] = "Voltar ao modo normal",
    [I18N_ACT_ENTER_COMMAND_DESC] = "Abrir prompt de comando",
    [I18N_ACT_ENTER_SEARCH_DESC] = "Abrir prompt de busca",
    [I18N_ACT_SEND_REQUEST_DESC] = "Enviar requisição atual",
    [I18N_ACT_TOGGLE_EDITOR_FIELD_DESC] = "Alternar campo do editor",
    [I18N_ACT_CYCLE_METHOD_DESC] = "Alternar método HTTP",
    [I18N_ACT_CYCLE_ENV_DESC] = "Alternar ambiente ativo",
    [I18N_ACT_HISTORY_LOAD_DESC] = "Carregar item selecionado do histórico",
    [I18N_ACT_HISTORY_REPLAY_DESC] = "Reexecutar requisição selecionada do histórico",
    [I18N_ACT_SEARCH_NEXT_DESC] = "Ir para a próxima ocorrência da busca",
    [I18N_ACT_SEARCH_PREV_DESC] = "Ir para a ocorrência anterior da busca",
};

const char *i18n_get(UiLanguage lang, I18nKey key) {
    if ((int)key < 0 || key >= I18N_COUNT) return "UNKNOWN";

    const char *pt = PT[key];
    const char *en = EN[key];
    if (lang == UI_LANG_PT && pt) return pt;
    if (en) return en;
    return "";
}

UiLanguage i18n_resolve_language(UiLanguageSetting setting, const char *lang_env) {
    if (setting == UI_LANG_SETTING_EN) return UI_LANG_EN;
    if (setting == UI_LANG_SETTING_PT) return UI_LANG_PT;
    if (starts_with_pt(lang_env)) return UI_LANG_PT;
    return UI_LANG_EN;
}

int i18n_parse_language_setting(const char *value, UiLanguageSetting *out) {
    if (!value || !out) return 1;

    if (str_eq_ci(value, "auto")) {
        *out = UI_LANG_SETTING_AUTO;
        return 0;
    }
    if (str_eq_ci(value, "en")) {
        *out = UI_LANG_SETTING_EN;
        return 0;
    }
    if (str_eq_ci(value, "pt")) {
        *out = UI_LANG_SETTING_PT;
        return 0;
    }
    return 1;
}

const char *i18n_language_setting_name(UiLanguageSetting setting) {
    switch (setting) {
        case UI_LANG_SETTING_EN:
            return "en";
        case UI_LANG_SETTING_PT:
            return "pt";
        case UI_LANG_SETTING_AUTO:
        default:
            return "auto";
    }
}
