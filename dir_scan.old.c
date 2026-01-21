#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include "headers/dir_util.h"
#include "headers/dir_scan.h"

#include "../includes/linux.h"

#include "../lib-util/headers/str_util.h"
#include "../lib-util/headers/file_util.h"
#include "../lib-table/headers/module_table.h"

static dir_ctx_t *DirScanCtx(dir_ctx_t *ctx, dir_config_t *conf) {
    dir_ctx_t *nctx = calloc(1, sizeof(dir_ctx_t));

    char path[PATH_MAX] = {0};

    nctx->scan_dirs = NULL;
    nctx->scan_dirs_len = 0;

    for(size_t i = 0; i < ctx->scan_dirs_len; i++) {
        if(((size_t)char_count(ctx->scan_dirs[i], '/') > conf->max_hiearchy_path) && (int)conf->max_hiearchy_path != -1) {
            if(ctx->total_scans > 0)
                cleanup_arr(ctx->scan_dirs, ctx->scan_dirs_len);
            else
                free(ctx->scan_dirs);

            cleanup_arr(nctx->scan_dirs, nctx->scan_dirs_len);

            free(ctx);
            free(nctx);
            return NULL;
        }

        DIR *dir;
        if((dir = opendir(ctx->scan_dirs[i])) == NULL)
            continue;
        
        struct dirent *files;

        while((files = readdir(dir))) {
            if(ctx->total_scan_dirs >= conf->total_dirs && (int)conf->total_dirs != -1) {
                if(ctx->total_scans > 0)
                    cleanup_arr(ctx->scan_dirs, ctx->scan_dirs_len);
                else
                    free(ctx->scan_dirs);

                cleanup_arr(nctx->scan_dirs, nctx->scan_dirs_len);

                free(ctx);
                free(nctx);
                return NULL;
            }
            if(!_strcmp(files->d_name, ".") || !_strcmp(files->d_name, ".."))
                continue;

            _memset(path, 0, PATH_MAX);

            _strcpy(path, ctx->scan_dirs[i]);

            if(_strcmp(ctx->scan_dirs[i], "/"))
                _strcat(path, "/");
            
            _strcat(path, files->d_name);

            if(_strstr(path, ds_table[DS_PROC].str))
                break;
            if(_strstr(path, ds_table[DS_DEVFD].str))
                continue;

            if(files->d_type == DT_DIR) {
                nctx->scan_dirs = realloc(nctx->scan_dirs, sizeof(char *) * (nctx->scan_dirs_len + 1));
                nctx->scan_dirs[nctx->scan_dirs_len++] = _strdup(path);

                if(FileCanWrite(path)) {
                    conf->fptr(path);
                    ctx->total_scan_dirs++;
                }
            }
        }

        closedir(dir);
    }

    nctx->total_scans = ctx->total_scans;

    if(!nctx->total_scans++) {
        free(ctx->scan_dirs);
        free(ctx);
    }
    else {
        cleanup_arr(ctx->scan_dirs, ctx->scan_dirs_len);
        free(ctx);
    }

    return nctx;    
}

static dir_ctx_t *DirScanFile(dir_ctx_t *ctx, dir_config_t *conf) {
    dir_ctx_t *nctx = calloc(1, sizeof(dir_ctx_t));

    char path[PATH_MAX] = {0};

    nctx->scan_dirs = NULL;
    nctx->scan_dirs_len = 0;

    for(size_t i = 0; i < ctx->scan_dirs_len; i++) {
        if(((size_t)char_count(ctx->scan_dirs[i], '/') > conf->max_hiearchy_path) && (int)conf->max_hiearchy_path != -1) {
            if(ctx->total_scans > 0)
                cleanup_arr(ctx->scan_dirs, ctx->scan_dirs_len);
            else
                free(ctx->scan_dirs);

            cleanup_arr(nctx->scan_dirs, nctx->scan_dirs_len);

            free(ctx);
            free(nctx);
            return NULL;
        }

        DIR *dir;
        if((dir = opendir(ctx->scan_dirs[i])) == NULL)
            continue;
        
        struct dirent *files;

        while((files = readdir(dir))) {
            if(ctx->total_scan_dirs >= conf->total_dirs && (int)conf->total_dirs != -1) {
                if(ctx->total_scans > 0)
                    cleanup_arr(ctx->scan_dirs, ctx->scan_dirs_len);
                else
                    free(ctx->scan_dirs);

                cleanup_arr(nctx->scan_dirs, nctx->scan_dirs_len);

                free(ctx);
                free(nctx);
                return NULL;
            }
            if(!_strcmp(files->d_name, ".") || !_strcmp(files->d_name, ".."))
                continue;

            _memset(path, 0, PATH_MAX);

            _strcpy(path, ctx->scan_dirs[i]);

            if(_strcmp(ctx->scan_dirs[i], "/"))
                _strcat(path, "/");
            
            _strcat(path, files->d_name);

            if(files->d_type != DT_DIR) {
                conf->fptr(path);
                ctx->total_scan_dirs++;
            } else {
                nctx->scan_dirs = realloc(nctx->scan_dirs, sizeof(char *) * (nctx->scan_dirs_len + 1));
                nctx->scan_dirs[nctx->scan_dirs_len++] = _strdup(path);
            }
        }

        closedir(dir);
    }

    nctx->total_scans = ctx->total_scans;

    if(!nctx->total_scans++) {
        free(ctx->scan_dirs);
        free(ctx);
    }
    else {
        cleanup_arr(ctx->scan_dirs, ctx->scan_dirs_len);
        free(ctx);
    }

    return nctx;    
}

void DirScan(dir_config_t *conf) {
    dir_ctx_t *ctx = calloc(1, sizeof(dir_ctx_t));

    if(conf->start == NULL && conf->start_arr == NULL)
        return;

    if(conf->start_arr == NULL) {
        ctx->scan_dirs = calloc(1, sizeof(char *)); 
        ctx->scan_dirs[0] = conf->start;
        ctx->scan_dirs_len = 1;
    } else {
        ctx->scan_dirs = calloc(conf->start_arr_len, sizeof(char *));
        ctx->scan_dirs_len = conf->start_arr_len;

        for(size_t i = 0; i < conf->start_arr_len; i++)
            ctx->scan_dirs[i] = conf->start_arr[i];
    }

    do {
        ctx = DirScanCtx(ctx, conf);

        if(ctx == NULL)
            break;

    } while(ctx != NULL);
}

void DirScanFiles(dir_config_t *conf) {
    dir_ctx_t *ctx = calloc(1, sizeof(dir_ctx_t));

    if(conf->start == NULL && conf->start_arr == NULL)
        return;

    if(conf->start_arr == NULL) {
        ctx->scan_dirs = calloc(1, sizeof(char *)); 
        ctx->scan_dirs[0] = conf->start;
        ctx->scan_dirs_len = 1;
    } else {
        ctx->scan_dirs = calloc(conf->start_arr_len, sizeof(char *));
        ctx->scan_dirs_len = conf->start_arr_len;

        for(size_t i = 0; i < conf->start_arr_len; i++)
            ctx->scan_dirs[i] = conf->start_arr[i];
    }

    do {
        ctx = DirScanFile(ctx, conf);

        if(ctx == NULL)
            break;

    } while(ctx != NULL);
}
