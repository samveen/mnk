/*
 * See LICENSE for details
 */

#include <hal/hal_gpio.h>
#include <stdint.h>

# define MNK_MATRIX_ROW_COUNT 6
# define MNK_MATRIX_COL_COUNT 6

/* typedef void (*hal_gpio_irq_handler_t)(void *arg); */

/* GPIO PIN associations */
int keyboard_output_pins[MNK_MATRIX_ROW_COUNT] = {0,1,2,3,4,5};
int keyboard_input_pins_set_left[MNK_MATRIX_COL_COUNT] = {6,7,8,9,10,11 };
int keyboard_input_pins_set_right[MNK_MATRIX_COL_COUNT] = {12,13,14,15,16,17};

void init_keyboard(void) {
    for (int pin=0;pin<MNK_MATRIX_ROW_COUNT;++pin) {
        // * @param val Value to set pin (0:low 1:high)
        hal_gpio_init_out(keyboard_output_pins[pin], 0);
    }
    for (int pin=0;pin<MNK_MATRIX_COL_COUNT;++pin) {
        hal_gpio_init_in(keyboard_input_pins_set_left[pin], HAL_GPIO_PULL_UP);
        hal_gpio_init_in(keyboard_input_pins_set_right[pin], HAL_GPIO_PULL_UP);
    }
}

void sendkeys(uint8_t matrix_left[MNK_MATRIX_ROW_COUNT][MNK_MATRIX_COL_COUNT], uint8_t matrix_right[MNK_MATRIX_ROW_COUNT][MNK_MATRIX_COL_COUNT]) {
}

void scan_keys(void) {
    /* Matrix state */
    uint8_t matrix_left[MNK_MATRIX_ROW_COUNT][MNK_MATRIX_COL_COUNT];
    uint8_t matrix_right[MNK_MATRIX_ROW_COUNT][MNK_MATRIX_COL_COUNT];

    /* Basic Scan */
    for(int opin=0; opin<MNK_MATRIX_ROW_COUNT; ++opin) {
        hal_gpio_write(keyboard_output_pins[opin], 1);
        for(int ipin=0; ipin<MNK_MATRIX_COL_COUNT; ++ipin) {
            matrix_left[opin][ipin] = hal_gpio_read(keyboard_input_pins_set_left[ipin]);
            matrix_right[opin][ipin] = hal_gpio_read(keyboard_input_pins_set_right[ipin]);
        }
        hal_gpio_write(keyboard_output_pins[opin], 0);
    }
    sendkeys(matrix_left,matrix_right);
}
