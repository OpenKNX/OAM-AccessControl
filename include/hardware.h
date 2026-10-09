#include <HardwareConfig.h>

#ifdef BOARD_XIAO_ESP32S3_HOMEKEY
    // Compile prototype: external KNX transceiver on XIAO D6/D7.
    // Confirm wiring before using this target on hardware.
    #define KNX_UART_NUM 1
    #define KNX_UART_TX_PIN 43
    #define KNX_UART_RX_PIN 44
    #define PROG_LED_PIN 21
    #define PROG_LED_PIN_ACTIVE_ON LOW
    #define OPENKNX_SWA_SET_ACTIVE_ON LOW
    #define OPENKNX_SWA_RESET_ACTIVE_ON LOW
    #define OPENKNX_SWA_BISTABLE_IMPULSE_LENGTH 50

    // Optional peripherals remain absent until prototype wiring is selected.
    // Do not inherit the RP2040 scanner, NFC, keypad or actuator pins.

    // PN7160 I2C prototype: XIAO D4/D5 bus, D2 IRQ, D3 VEN.
    #define OPENKNX_GPIO_WIRE Wire
    #define OPENKNX_GPIO_SDA 5
    #define OPENKNX_GPIO_SCL 6
    #define OPENKNX_GPIO_CLOCK 400000
    #define NFC_PN7160_ADDR 0x28
    #define NFC_IRQ_PIN 3
    #define NFC_VEN_PIN 4

    #ifdef OPENKNX_HOMEKEY_ENABLED
        // PN7161 SPI prototype, mutually exclusive with the peripheral test profile.
        #ifdef OPENKNX_ACC_TEST_PERIPHERALS
            #error Home Key SPI pins conflict with the optional peripheral profile
        #endif
        #define HOMEKEY_SPI_CS 1
        #define HOMEKEY_SPI_SCLK 7
        #define HOMEKEY_SPI_MISO 8
        #define HOMEKEY_SPI_MOSI 9
        #define HOMEKEY_IRQ_PIN 3
        #define HOMEKEY_VEN_PIN 4
    #endif

    #ifdef OPENKNX_ACC_TEST_PERIPHERALS
        #define SCANNER_SERIAL Serial2
        #define SCANNER_SERIAL_RX_PIN 1
        #define SCANNER_SERIAL_TX_PIN 2
        #define SCANNER_TOUCH_PIN 7
        #define OPENKNX_GPIO_NUM 3
        #define OPENKNX_GPIO_TYPES OPENKNX_GPIO_T_PCA9554, OPENKNX_GPIO_T_PCA9554, OPENKNX_GPIO_T_PCA9554
        #define OPENKNX_GPIO_ADDRS 0x21, 0x20, 0x22
        #define OPENKNX_GPIO_INTS 0xFF, 0xFF, 0xFF
        #define DIRECT_TOUCH_LEFT_PIN 0x0104
        #define DIRECT_TOUCH_RIGHT_PIN 0x0107
        #define DIRECT_LED_GREEN_PIN 0x0105
        #define DIRECT_LED_RED_PIN 0x0106
        #define EXTERN_TOUCH_LEFT_PIN 0x0204
        #define EXTERN_TOUCH_RIGHT_PIN 0x0207
        #define EXTERN_LED_GREEN_PIN 0x0205
        #define EXTERN_LED_RED_PIN 0x0206
        #define KEYPAD_PCA9633_ADDR 0x62
        #define OPENKNX_ACC_KEYPAD_ROW_PINS 0x0300, 0x0301, 0x0302, 0x0303
        #define OPENKNX_ACC_KEYPAD_COL_PINS 0x0304, 0x0305, 0x0306
        #define OPENKNX_SWA_CHANNEL_COUNT 1
        #define OPENKNX_SWA_SET_PINS 8
        #define OPENKNX_SWA_RESET_PINS 9
        #define OPENKNX_BI_GPIO_COUNT 4
        #define OPENKNX_BI_GPIO_PINS 0x0100, 0x0101, 0x0102, 0x0103
    #else
        #define OPENKNX_SWA_CHANNEL_COUNT 0
        #define OPENKNX_BI_GPIO_COUNT 0
    #endif
#endif

#ifdef BOARD_AB_SMARTHOUSE_FINGERPRINT_V13
    #define INFO_LED_PIN 11
    #define INFO_LED_PIN_ACTIVE_ON HIGH
    #define PROG_LED_PIN 10
    #define PROG_LED_PIN_ACTIVE_ON HIGH
    #define PROG_BUTTON_PIN 9
    #define PROG_BUTTON_PIN_INTERRUPT_ON FALLING

    #define KNX_UART_NUM 0
    #define KNX_UART_TX_PIN 12
    #define KNX_UART_RX_PIN 13
    #define SAVE_INTERRUPT_PIN 0

    #define SCANNER_SERIAL Serial2
    #define SCANNER_SERIAL_RX_PIN 5
    #define SCANNER_SERIAL_TX_PIN 4
    #define SCANNER_PWR_PIN 1
    #define SCANNER_PWR_PIN_ACTIVE_ON LOW
    #define SCANNER_TOUCH_PIN 2

    #define DIRECT_TOUCH_LEFT_PIN 26
    #define DIRECT_TOUCH_RIGHT_PIN 27
    #define DIRECT_LED_GREEN_PIN 24
    #define DIRECT_LED_RED_PIN 25
    #define EXTERN_TOUCH_LEFT_PIN 0x0204
    #define EXTERN_TOUCH_RIGHT_PIN 0x0207
    #define EXTERN_LED_GREEN_PIN 0x0205
    #define EXTERN_LED_RED_PIN 0x0206

    #define OPENKNX_SWA_CHANNEL_COUNT 1
    #define OPENKNX_SWA_SET_PINS 14
    #define OPENKNX_SWA_RESET_PINS 15
    #define OPENKNX_SWA_SET_ACTIVE_ON LOW
    #define OPENKNX_SWA_RESET_ACTIVE_ON LOW
    #define OPENKNX_SWA_BISTABLE_IMPULSE_LENGTH 50

    #define OPENKNX_GPIO_NUM 2
    #define OPENKNX_GPIO_TYPES OPENKNX_GPIO_T_EMBEDDED, OPENKNX_GPIO_T_PCA9554
    #define OPENKNX_GPIO_ADDRS 0xFF, 0x20
    #define OPENKNX_GPIO_INTS 0xFF, 0xFF

    #define OPENKNX_GPIO_WIRE Wire
    #define OPENKNX_GPIO_SDA 20
    #define OPENKNX_GPIO_SCL 21
    #define OPENKNX_GPIO_CLOCK 400000

    #define NFC_PN7160_ADDR 0x28
    #define NFC_IRQ_PIN 16
    #define NFC_VEN_PIN 17
    #define NFC_DWL_REQ_PIN 23

    #define KEYPAD_PCA9633_ADDR 0x62 //(0xc0 >> 1)

    #define OPENKNX_BI_GPIO_PINS 28, 18, 29, 19
    #define OPENKNX_BI_GPIO_COUNT 4
#endif

#ifdef DEVICE_UP1_SEN_8X
    #define SCANNER_SERIAL Serial2
    #define SCANNER_SERIAL_RX_PIN OKNXHW_SENSOR_E1_SCL_PIN
    #define SCANNER_SERIAL_TX_PIN OKNXHW_SENSOR_E2_SDA_PIN
    #define SCANNER_TOUCH_PIN OKNXHW_SENSOR_F1_SCL_PIN
    #define DIRECT_TOUCH_LEFT_PIN -1
    #define DIRECT_TOUCH_RIGHT_PIN -1
    #define DIRECT_LED_GREEN_PIN -1
    #define DIRECT_LED_RED_PIN -1

    #define EXTERN_TOUCH_LEFT_PIN -1
    #define EXTERN_TOUCH_RIGHT_PIN -1
    #define EXTERN_LED_GREEN_PIN -1
    #define EXTERN_LED_RED_PIN -1

    #define NFC_IRQ_PIN -1
    #define NFC_VEN_PIN -1
    #define NFC_PN7160_ADDR 0

    #define OPENKNX_SWA_CHANNEL_COUNT 0
    #define OPENKNX_SWA_SET_ACTIVE_ON LOW
    #define OPENKNX_SWA_RESET_ACTIVE_ON LOW
    #define OPENKNX_SWA_BISTABLE_IMPULSE_LENGTH 50

    #define OPENKNX_BI_GPIO_PINS OKNXHW_SENSOR_G2_SDA_PIN, OKNXHW_SENSOR_G1_SCL_PIN, OKNXHW_SENSOR_G1_SCL_PIN, OKNXHW_SENSOR_G2_SDA_PIN
    #define OPENKNX_BI_GPIO_COUNT 4

    #define OPENKNX_ACC_KEYPAD_ROW_PINS OKNXHW_SENSOR_A1_SCL_PIN, OKNXHW_SENSOR_A2_SDA_PIN, OKNXHW_SENSOR_B1_SCL_PIN, OKNXHW_SENSOR_B2_SDA_PIN
    #define OPENKNX_ACC_KEYPAD_COL_PINS OKNXHW_SENSOR_C1_SCL_PIN, OKNXHW_SENSOR_C2_SDA_PIN, OKNXHW_SENSOR_D1_SCL_PIN

    #define OPENKNX_BUZZER_PIN OKNXHW_SENSOR_D2_SDA_PIN
#endif
