#ifndef PICO_BNO08X_H
#define PICO_BNO08X_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "sh2.h"
#include "sh2_err.h"
#include "sh2_hal.h"
#include "sh2_SensorValue.h"

#ifdef __cplusplus
extern "C" {
#endif

    // Global debug control flag (defined exactly once in Pico_BNO08x_multi.c)
    extern bool debug_suppressed;

    /**
     * @brief BNO08x device state and configuration for a single instance
     */
    typedef struct {
        // SPI configuration
        spi_inst_t* spi_port;
        uint8_t cs_pin;
        uint8_t int_pin;
        uint8_t reset_pin;
        uint32_t spi_frequency;
        int instance_id;

        uint8_t mosi_pin;
        uint8_t miso_pin;
        uint8_t sck_pin;

        // SH2 instance
        sh2_Instance_t* sh2_instance;
        sh2_Hal_t hal;

        // Sensor buffering
        sh2_SensorValue_t sensor_value;
        sh2_SensorValue_t* pending_value;

        // State
        bool initialized;
        bool spi_begun;
        bool has_reset;

        // Timing
        uint32_t last_reset_time_us;
        uint32_t last_service_time_us;

    } Pico_BNO08x_t;

    // ==== Public API ====

    bool pico_bno08x_init(Pico_BNO08x_t* bno, int reset_pin, int instance_id);

    bool pico_bno08x_begin_spi(Pico_BNO08x_t* bno,
        spi_inst_t* spi_port,
        uint8_t miso_pin,
        uint8_t mosi_pin,
        uint8_t sck_pin,
        uint8_t cs_pin,
        uint8_t int_pin,
        uint32_t frequency);

    void pico_bno08x_service(Pico_BNO08x_t* bno);

    bool pico_bno08x_enable_report(Pico_BNO08x_t* bno,
        sh2_SensorId_t sensor_id,
        uint32_t interval_us);

    bool pico_bno08x_disable_report(Pico_BNO08x_t* bno,
        sh2_SensorId_t sensor_id);

    bool pico_bno08x_get_sensor_event(Pico_BNO08x_t* bno,
        sh2_SensorValue_t* value);

    bool pico_bno08x_data_available(Pico_BNO08x_t* bno);

    void pico_bno08x_reset(Pico_BNO08x_t* bno);

    bool pico_bno08x_soft_reset(Pico_BNO08x_t* bno);

    void pico_bno08x_destroy(Pico_BNO08x_t* bno);

#ifdef __cplusplus
}
#endif

#endif // PICO_BNO08X_H