#include "mr24hpc.h"

void my_state_callback(const mr24hpc_state_t *state) {
    // TODO: napisi callback funkciju koja obrađuje novo stanje senzora
    return;
}

void app_main(void) {
    mr24hpc_init();
    mr24hpc_register_callback(my_state_callback);
    mr24hpc_start();

    mr24hpc_state_t state;
    while(1) {
        if(mr24hpc_get_state(&state)) {
            // obrada
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}