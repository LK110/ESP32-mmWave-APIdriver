#include "mr24hpc.h"

void app_main(void) {
    mr24hpc_init();
    mr24hpc_start();

    mr24hpc_state_t state;
    while(1) {
        if(mr24hpc_get_state(&state)) {
            // obrada
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}