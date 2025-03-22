#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void adios_ctx_yield();
void adios_ctx_yield_global();
int adios_ctx_id();
char adios_ctx_worker_id();
void adios_ctx_set_cls(void *v);
void *adios_ctx_get_cls();

#ifdef __cplusplus
}
#endif