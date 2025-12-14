#include "mr24hpc_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <esp_err.h>

typedef void (*mr24hpc_callback_function)(const mr24hpc_state_t *state); //pokazivac na funkciju: (mr24hpc_state_t*) -> void

esp_err_t mr24hpc_init(void);
esp_err_t mr24hpc_start(void);
//esp_err_t mr24hpc_stop(void);

bool mr24hpc_get_state(mr24hpc_state_t *state_copy);
esp_err_t mr24hpc_register_callback(mr24hpc_callback_function cb_fuction);
