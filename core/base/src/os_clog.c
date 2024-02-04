/************************************************************************
 *File name: os_clog.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.02
************************************************************************/
#include "os_init.h"
#include "private/os_clog_priv.h"


const char* g_logStr[MAX_LOG_LEVEL] = {
   NULL, "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
};

/* number of times log function is called */
int	g_nWrites = 0; 

/* Socke descriptor if remote client is connected */
int g_nCliSocket = 0;


typedef struct os_log_domain_s {
    os_lnode_t node;

    int id;
    os_log_level_e level;
    const char *name;
} os_log_domain_t;

PRIVATE OS_POOL(domain_pool, os_log_domain_t);
PRIVATE OS_LIST(domain_list);

_API_ void os_clog_domain_init(void)
{
    os_pool_init(&domain_pool, os_global_context()->log.domain_pool);
}

_API_ void os_clog_domain_final(void)
{
    os_log_domain_t *domain, *saved_domain;

    os_list_for_each_safe(&domain_list, saved_domain, domain)
        os_log_remove_domain(domain);
    os_pool_final(&domain_pool);
}

os_log_domain_t *os_log_add_domain(const char *name, os_log_level_e level)
{
    os_log_domain_t *domain = NULL;

    os_assert(name);

    os_pool_alloc(&domain_pool, &domain);
    os_assert(domain);

    domain->name = name;
    domain->id = os_pool_index(&domain_pool, domain);
    domain->level = level;

    os_list_add(&domain_list, domain);

    return domain;
}

os_log_domain_t *os_log_find_domain(const char *name)
{
    os_log_domain_t *domain = NULL;

    os_assert(name);

    os_list_for_each(&domain_list, domain)
        if (!os_strcasecmp(domain->name, name))
            return domain;

    return NULL;
}

void os_log_remove_domain(os_log_domain_t *domain)
{
    os_assert(domain);

    os_list_remove(&domain_list, domain);
    os_pool_free(&domain_pool, domain);
}

void os_log_set_domain_level(int id, os_log_level_e level)
{
    os_log_domain_t *domain = NULL;

    os_assert(id > 0 && id <= os_global_context()->log.domain_pool);

    domain = os_pool_find(&domain_pool, id);
    os_assert(domain);

    domain->level = level;
}

os_log_level_e os_log_get_domain_level(int id)
{
    os_log_domain_t *domain = NULL;

    os_assert(id > 0 && id <= os_global_context()->log.domain_pool);

    domain = os_pool_find(&domain_pool, id);
    os_assert(domain);

    return domain->level;
}

const char *os_log_get_domain_name(int id)
{
    os_log_domain_t *domain = NULL;

    os_assert(id > 0 && id <= os_global_context()->log.domain_pool);

    domain = os_pool_find(&domain_pool, id);
    os_assert(domain);

    return domain->name;
}

int os_log_get_domain_id(const char *name)
{
    os_log_domain_t *domain = NULL;

    os_assert(name);
    
    domain = os_log_find_domain(name);
    os_assert(domain);

    return domain->id;
}

_API_ void os_log_install_domain(int *domain_id, const char *name, os_log_level_e level)
{
    os_log_domain_t *domain = NULL;

    os_assert(domain_id);
    os_assert(name);

    domain = os_log_find_domain(name);
    if (domain) {
        os_logs(WARN, "`%s` log-domain duplicated", name);
        if (level != domain->level) {
            os_logs(WARN, ">>>[%s]", g_logStr[domain->level]);
	        os_logs(WARN, "[%s]log-level changed<<<", g_logStr[level]);
            os_log_set_domain_level(domain->id, level);
        }
    } else {
        domain = os_log_add_domain(name, level);
        os_assert(domain);
    }

    *domain_id = domain->id;
}

void os_log_set_mask_level(const char *_mask, os_log_level_e level)
{
    os_log_domain_t *domain = NULL;

    if (_mask) {
        const char *delim = " \t\n,:";
        char *mask = NULL;
        char *saveptr;
        char *name;

        mask = os_strdup(_mask);
        os_assert(mask);

        for (name = os_strtok_r(mask, delim, &saveptr);
            name != NULL;
            name = os_strtok_r(NULL, delim, &saveptr)) {

            domain = os_log_find_domain(name);
            if (domain)
                domain->level = level;
        }

        os_free(mask);
    } else {//all
        os_list_for_each(&domain_list, domain)
            domain->level = level;
    }
}

PRIVATE os_log_level_e os_log_level_from_string(const char *string)
{
    os_log_level_e level = OS_ERROR;

    if (!strcasecmp(string, "none")) level = NONE;
    else if (!strcasecmp(string, "fatal")) level = FATAL;
    else if (!strcasecmp(string, "error")) level = ERROR;
    else if (!strcasecmp(string, "warn")) level = WARN;
    else if (!strcasecmp(string, "info")) level = INFO;
    else if (!strcasecmp(string, "debug")) level = DEBUG;
    else if (!strcasecmp(string, "trace")) level = TRACE;

    return level;
}

_API_ int os_log_config_domain(const char *domain_mask, const char *level)
{
    if (domain_mask || level) {
        int l = os_global_context()->log.level;

        if (level) {
            l = os_log_level_from_string(level);
            if (l == OS_ERROR) {
                os_log1(ERROR, "Invalid LOG-LEVEL "
                        "[none:fatal|error|warn|info|debug|trace]: %s\n",
                        level);
                return OS_ERROR;
            }
        }

        os_log_set_mask_level(domain_masks, l);
    }

    return OS_OK;
}


void hex2Asii(char* p, const char* h, int hexlen)
{
	for(int i = 0; i < hexlen; i++, p+=3, h++){
		sprintf(p, "%02x ", *h);
	}
}


