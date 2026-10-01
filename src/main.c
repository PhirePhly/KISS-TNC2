#include "tnc2.h"

const char firmware_version[] = "KISS TNC-2 C v1.0";

static void boot_led_dance(void)
{
    uint8_t flashes;
    bool on = false;

    for (flashes = 0; flashes < 6u; ++flashes) {
        volatile uint16_t delay = 0;
        on = !on;
        hardware_sta(on);
        hardware_con(on);
        do {
            --delay;
        } while (delay);
    }
    hardware_sta(false);
    hardware_con(false);
}

void firmware_boot(void)
{
    hardware_irq_disable();
    state_reset();
    buffers_init();
    hardware_init();
    boot_led_dance();

    /* Flush any SIO state accumulated during the visible power-on test. */
    hardware_init();
    hardware_set_im2();

    for (;;) {
        modem_service();
        host_service();
    }
}
