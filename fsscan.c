#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>

typedef void (*fsscan_f)(const char *path);

typedef struct {
    int mode;
    int ftype;

    size_t fptr_limit;
    size_t nest_limit;
    size_t scan_limit;

    const char **blacklist;
    size_t blacklist_size;

    const char **whitelist;
    size_t whitelist_size;

    bool start_enabled;

    const char **start;
    size_t start_size;

    fsscan_f func;
} fsscan_config_t;

typedef struct {
    size_t fptr_used;
    size_t scan_used;
    size_t prev_nest;

    char **lscan;
    size_t lscan_size;
} fsscan_ctx_t;

static const char *blacklist[] = {
    "/proc",
    "/dev/fd",
};

#define BLACKLIST_SIZE 2

void copy_array_memory(void *dest, size_t dest_limit, const char **src, size_t src_limit) {
    if(src_limit > dest_limit)
        return;

    memcpy(dest, src, dest_limit * sizeof(char *));
}

void copy_array(char **dest, size_t dest_limit, char **src, size_t src_limit) {
    for(size_t i = 0; i < dest_limit; i++) {
        dest[i] = src[i];
    }
}

void dupe_string_array(char **dest, size_t dest_limit, const char **src, size_t src_limit) {
    for(size_t i = 0; i < dest_limit; i++) {
        dest[i] = strdup(src[i]);
    }
}

void free_array(char **dest, size_t elem) {
    for(size_t i = 0; i < elem; i++)
        free(dest[i]);
    free(dest);
    dest = NULL;
}

int count_char(const char *path, char cr) {
    int c = 0;
    while(*path) {
        if(*path++ == cr)
            c++;
    }
    return c;
}

int strstart(const char *s1, const char *s2) {
    while(*s1++) {
        if(*s1 != *s2++)
            return 0;
    }
    return 1;
}

bool strstart_array(const char **array, size_t size, const char *string) {
    for(size_t i = 0; i < size; i++) {
        if(strstart(array[i], string))
            return true;
    }
    return false;
}

bool strstr_array(const char **array, size_t size, const char *string) {
    for(size_t i = 0; i < size; i++) {
        if(strstr(array[i], string))
            return true;
    }
    return false;
}

static int fsscan_next(fsscan_ctx_t *ctx, fsscan_config_t *config) {
    if(ctx->lscan == NULL || config == NULL || ctx->lscan_size == 0) {
        return -1;
    }

    mode_t ftype;
    struct stat st;

    size_t total = 0;
    char **new_used = NULL;

    for(size_t i = 0; i < ctx->lscan_size; i++) {
        if(config->nest_limit != -1 && count_char(ctx->lscan[i], '/') >= config->nest_limit) {
            continue;
        }

        if(config->blacklist_size > 0 && strstart_array(config->blacklist, config->blacklist_size, ctx->lscan[i])) {
            free_array(new_used, total);
            return 0;
        }

        if(config->whitelist_size > 0 && !strstart_array(config->whitelist, config->whitelist_size, ctx->lscan[i])) {
            free_array(new_used, total);
            return 0;
        }

        DIR *dptr;
        if((dptr = opendir(ctx->lscan[i])) == NULL)
            continue;
        
        struct dirent *files;

        while((files = readdir(dptr))) {
            if(!strcmp(files->d_name, ".") || !strcmp(files->d_name, "..")) {
                continue;
            }

            if(config->scan_limit != -1 && ctx->scan_used >= config->scan_limit) {
                closedir(dptr);
                free_array(new_used, total);
                return 0;
            }

            ctx->scan_used++;

            if(BLACKLIST_SIZE > 0 && strstart_array(blacklist, BLACKLIST_SIZE, ctx->lscan[i])) {
                closedir(dptr);
                free_array(new_used, total);
                return 0;
            }

            const char *folder_path = ctx->lscan[i];
            char cpath[4096] = {0};

            /* 
            folder_path/files->d_name 
            */
            
            strcpy(cpath, folder_path);   
            if(strcmp(folder_path, "/"))
                strcat(cpath, "/");
            strcat(cpath, files->d_name);

            if(ctx->fptr_used + 1 > config->fptr_limit) {
                closedir(dptr);
                free_array(new_used, total);
                return 0;
            }

            /* add to updated paths */
            new_used = realloc(new_used, (total + 1) * sizeof(char *));
            new_used[total++] = strdup(cpath);

            if(config->mode < 0 && config->ftype < 0) {
                ctx->fptr_used++;
                config->func(cpath);
                continue;
            }

            if(stat(cpath, &st) != 0) {
                continue;
            }

            ftype = st.st_mode & S_IFMT;

            if(config->ftype > 0 && ftype != config->ftype) {
                continue;
            }

            if(config->mode > 0 && access(cpath, config->mode) != 0) {
                continue;
            }

            ctx->fptr_used++;
            config->func(cpath);
        }

        closedir(dptr);
    }

    if(total == 0)
        return 0;

    free_array(ctx->lscan, ctx->lscan_size);
    
    ctx->lscan = new_used;
    ctx->lscan_size = total;

    return 1;
}

int fsscan(fsscan_config_t *config) {
    if(config == NULL) {
        return -1;
    }

    fsscan_ctx_t ctx = {0};

    if(config->start_enabled == true) {
        ctx.lscan = calloc(config->start_size, sizeof(char *));
        dupe_string_array(ctx.lscan, config->start_size, config->start, config->start_size);
    } else {
        ctx.lscan = calloc(1, sizeof(char *));
        ctx.lscan[0]    = calloc(1, 2);
        ctx.lscan[0][0] = '/';
        ctx.lscan_size  = 1;
    }
    
    int cont;

    do {
        cont = fsscan_next(&ctx, config);
    } while(cont == 1);

    free_array(ctx.lscan, ctx.lscan_size);

    return ctx.scan_used;
}

void fptr_test(const char *path) {
    printf("Read path -> %s\r\n", path);
}

int main(void) {
    int ret = fsscan(&(fsscan_config_t){
        .mode = 0,
        .ftype = S_IFREG,

        .start_enabled = false,
        .start = NULL,

        .nest_limit = 3,
        .scan_limit = -1,
        .fptr_limit = -1,

        .func = fptr_test,
    });

    printf("Scanned %d files\n", ret);
}

