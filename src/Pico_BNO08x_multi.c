#include "Pico_BNO08x.h"

#include <string.h>
#include <stddef.h>

#include "hardware/gpio.h"

// =======================
// Global debug flag
// =======================
bool debug_suppressed = false;

// =======================
// Constants
// =======================
#define INT_TIMEOUT_MS   3000
#define RESET_DELAY_MS    50
#define BOOT_DELAY_MS    500

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

// =======================
// Forward declarations (PRIVATE)
// =======================
static void hardware_reset(Pico_BNO08x_t* bno);
static uint32_t hal_get_time_us(sh2_Hal_t* self);

static void hal_callback(void* cookie, sh2_AsyncEvent_t* e);
static void sensor_handler(void* cookie, sh2_SensorEvent_t* evt);

static bool spi_hal_wait_for_int(Pico_BNO08x_t* bno);
static int  spi_hal_open(sh2_Hal_t* self);
static void spi_hal_close(sh2_Hal_t* self);
static int  spi_hal_read(sh2_Hal_t* self, uint8_t* buf, unsigned len, uint32_t* t_us);
static int  spi_hal_write(sh2_Hal_t* self, uint8_t* buf, unsigned len);

// =======================
// Public API
// =======================

bool pico_bno08x_init(Pico_BNO08x_t* bno, int reset_pin, int instance_id) {
    if (!bno) return false;

    memset(bno, 0, sizeof(*bno));
    bno->reset_pin = reset_pin;
    bno->instance_id = instance_id;
    bno->spi_frequency = 3000000;

    bno->sh2_instance = sh2_createInstance();
    if (!bno->sh2_instance) return false;

    bno->hal.getTimeUs = hal_get_time_us;
    bno->hal.open = spi_hal_open;
    bno->hal.close = spi_hal_close;
    bno->hal.read = spi_hal_read;
    bno->hal.write = spi_hal_write;

    bno->pending_value = &bno->sensor_value;
    return true;
}

bool pico_bno08x_begin_spi(Pico_BNO08x_t* bno,
    spi_inst_t* spi,
    uint8_t miso,
    uint8_t mosi,
    uint8_t sck,
    uint8_t cs,
    uint8_t irq,
    uint32_t speed) {
    if (!bno || !spi || !bno->sh2_instance) return false;

    bno->spi_port = spi;
    bno->miso_pin = miso;
    bno->mosi_pin = mosi;
    bno->sck_pin = sck;
    bno->cs_pin = cs;
    bno->int_pin = irq;
    bno->spi_frequency = speed;

    gpio_set_function(miso, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);
    gpio_set_function(sck, GPIO_FUNC_SPI);

    spi_init(spi, speed);
    spi_set_format(spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

    gpio_init(cs);
    gpio_set_dir(cs, GPIO_OUT);
    gpio_put(cs, 1);

    gpio_init(irq);
    gpio_set_dir(irq, GPIO_IN);
    gpio_pull_up(irq);

    hardware_reset(bno);
    sleep_ms(BOOT_DELAY_MS);

    int rc = sh2_openInstance(bno->sh2_instance, &bno->hal, hal_callback, bno);
    if (rc != SH2_OK) return false;

    sh2_setSensorCallbackInstance(bno->sh2_instance, sensor_handler, bno);
    bno->initialized = true;

    return true;
}

void pico_bno08x_service(Pico_BNO08x_t* bno) {
    if (bno && bno->sh2_instance)
        sh2_serviceInstance(bno->sh2_instance);
}

bool pico_bno08x_enable_report(Pico_BNO08x_t* bno,
    sh2_SensorId_t id,
    uint32_t interval_us) {
    if (!bno || !bno->sh2_instance) return false;

    sh2_SensorConfig_t cfg = {
        .reportInterval_us = interval_us
    };

    return sh2_setSensorConfigInstance(bno->sh2_instance, id, &cfg) == SH2_OK;
}

bool pico_bno08x_get_sensor_event(Pico_BNO08x_t* bno,
    sh2_SensorValue_t* val) {
    if (!bno || !val) return false;

    bno->pending_value = val;
    val->timestamp = 0;

    sh2_serviceInstance(bno->sh2_instance);

    bno->pending_value = &bno->sensor_value;
    return (val->timestamp != 0);
}

void pico_bno08x_destroy(Pico_BNO08x_t* bno) {
    if (!bno) return;

    if (bno->sh2_instance) {
        sh2_closeInstance(bno->sh2_instance);
        sh2_destroyInstance(bno->sh2_instance);
        bno->sh2_instance = NULL;
    }
}

// =======================
// HAL + callbacks (PRIVATE)
// =======================

static uint32_t hal_get_time_us(sh2_Hal_t* self) {
    (void)self;
    return time_us_32();
}

static void hal_callback(void* cookie, sh2_AsyncEvent_t* e) {
    Pico_BNO08x_t* bno = (Pico_BNO08x_t*)cookie;
    if (e->eventId == SH2_RESET)
        bno->has_reset = true;
}

static void sensor_handler(void* cookie, sh2_SensorEvent_t* evt) {
    Pico_BNO08x_t* bno = (Pico_BNO08x_t*)cookie;
    if (sh2_decodeSensorEvent(bno->pending_value, evt) != SH2_OK)
        bno->pending_value->timestamp = 0;
}

static int spi_hal_open(sh2_Hal_t* self) {
    (void)self;
    return SH2_OK;
}

static void spi_hal_close(sh2_Hal_t* self) {
    (void)self;
}

static bool spi_hal_wait_for_int(Pico_BNO08x_t* bno) {
    for (int i = 0; i < 1000; i++) {
        if (!gpio_get(bno->int_pin)) return true;
        sleep_us(100);
    }
    return false;
}

static int spi_hal_read(sh2_Hal_t* self,
    uint8_t* buf,
    unsigned len,
    uint32_t* t_us) {
    Pico_BNO08x_t* bno = container_of(self, Pico_BNO08x_t, hal);
    spi_hal_wait_for_int(bno);

    gpio_put(bno->cs_pin, 0);
    int ret = spi_read_blocking(bno->spi_port, 0x00, buf, len);
    gpio_put(bno->cs_pin, 1);

    if (t_us) *t_us = time_us_32();
    return ret;
}

static int spi_hal_write(sh2_Hal_t* self,
    uint8_t* buf,
    unsigned len) {
    Pico_BNO08x_t* bno = container_of(self, Pico_BNO08x_t, hal);
    spi_hal_wait_for_int(bno);

    gpio_put(bno->cs_pin, 0);
    int ret = spi_write_blocking(bno->spi_port, buf, len);
    gpio_put(bno->cs_pin, 1);

    return ret;
}

static void hardware_reset(Pico_BNO08x_t* bno) {
    gpio_init(bno->reset_pin);
    gpio_set_dir(bno->reset_pin, GPIO_OUT);
    gpio_put(bno->reset_pin, 0);
    sleep_ms(RESET_DELAY_MS);
    gpio_put(bno->reset_pin, 1);
}