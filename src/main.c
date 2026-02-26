#include <stdio.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "Pico_BNO08x.h"
#include "wireless_transfer.h"

// SPI Configuration pins
#define SPI_PORT        spi0
#define SPI_MISO_PIN    16
#define SPI_MOSI_PIN    19
#define SPI_SCK_PIN     18

// IMU0 Pins (arm)
#define CS1_PIN         17
#define INT1_PIN        20
#define RESET1_PIN      15
// IMU1 Pins (shoulder)
#define CS2_PIN         13
#define INT2_PIN        12
#define RESET2_PIN      14
// IMU2 Pins (chest)
#define CS3_PIN         9
#define INT3_PIN        10
#define RESET3_PIN      11

// Timing parameters
#define SWITCH_INTERVAL_MS 1000    // switch every 1 second
#define POLL_DELAY_MS       40     // ms between polls

typedef struct {
    uint cs, inten, rst;
    const char* label;
} imu_cfg_t;

static const imu_cfg_t imus[3] = {
    {CS1_PIN, INT1_PIN, RESET1_PIN, "arm_imu"},
    {CS2_PIN, INT2_PIN, RESET2_PIN, "shoulder_imu"},
    {CS3_PIN, INT3_PIN, RESET3_PIN, "chest_imu"}
};

static Pico_BNO08x_t active;
static int cur = -1;
static bool available[3] = { false };
static uint64_t t0_ms = 0;
static float quat[3][4] = { {0} };  // w,x,y,z for each IMU

static void init_hw(void) {
    stdio_init_all();
    sleep_ms(2000);

    spi_init(SPI_PORT, 3000000);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);

    gpio_set_function(SPI_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);

    for (int i = 0; i < 3; i++) {
        gpio_init(imus[i].cs);
        gpio_set_dir(imus[i].cs, GPIO_OUT);
        gpio_put(imus[i].cs, 1);

        gpio_init(imus[i].inten);
        gpio_set_dir(imus[i].inten, GPIO_IN);
        gpio_pull_down(imus[i].inten);

        gpio_init(imus[i].rst);
        gpio_set_dir(imus[i].rst, GPIO_OUT);
        gpio_put(imus[i].rst, 0);
    }

    sleep_ms(100);
    for (int i = 0; i < 3; i++) {
        gpio_put(imus[i].rst, 1);
        sleep_ms(50);
    }
    sleep_ms(200);
}

static bool check_resp(int i) {
    uint8_t buf[4] = { 0 };
    gpio_put(imus[i].cs, 0);
    sleep_us(5);
    spi_read_blocking(SPI_PORT, 0, buf, 4);
    sleep_us(5);
    gpio_put(imus[i].cs, 1);

    for (int j = 0; j < 4; j++) {
        if (buf[j] != 0x00 && buf[j] != 0xFF) return true;
    }
    return false;
}

static bool select_imu(int i) {
    if (!available[i]) return false;

    if (cur >= 0) {
        pico_bno08x_destroy(&active);
    }

    const imu_cfg_t* c = &imus[i];

    if (!pico_bno08x_init(&active, c->rst, i)) return false;

    if (!pico_bno08x_begin_spi(&active, SPI_PORT,
        SPI_MISO_PIN, SPI_MOSI_PIN, SPI_SCK_PIN,
        c->cs, c->inten, 3000000)) return false;

    pico_bno08x_enable_report(&active, SH2_ROTATION_VECTOR, 50000);
    cur = i;
    return true;
}

int main(void) {
    init_hw();

    // Detect available IMUs
    int count = 0;
    for (int i = 0; i < 3; i++) {
        available[i] = check_resp(i);
        if (available[i]) count++;
        printf("%s: %s\n", imus[i].label, available[i] ? "OK" : "NOPE");
    }
    if (!count) {
        printf("No IMUs detected.\n");
        return -1;
    }

    // Initialize Wi-Fi/UDP (optional; runs alongside printing)
    bool wifi_ok = wireless_init();
    if (!wifi_ok) {
        printf("WiFi init failed; continuing without UDP.\n");
    }

    // Print OpenSim .sto header once
    printf("IMU_Data\n");
    printf("nRows=auto\n");
    printf("nColumns=13\n");
    printf("version=1\n");
    printf("inDegrees=no\n");
    printf("endheader\n");
    printf("time");
    for (int i = 0; i < 3; i++) {
        printf(",%s_w,%s_x,%s_y,%s_z",
            imus[i].label, imus[i].label, imus[i].label, imus[i].label);
    }
    printf("\n");

    // Pick first available IMU
    int next = 0;
    while (next < 3 && !available[next]) next++;
    if (next >= 3) return -1;

    select_imu(next);
    uint64_t last_switch_ms = to_ms_since_boot(get_absolute_time());
    t0_ms = last_switch_ms;
    next = (next + 1) % 3;

    while (true) {
        uint64_t now_ms = to_ms_since_boot(get_absolute_time());

        // Switch IMU periodically (skip unavailable targets)
        if (now_ms - last_switch_ms >= SWITCH_INTERVAL_MS) {
            int tries = 0;
            while (tries < 3 && !available[next]) { next = (next + 1) % 3; tries++; }
            if (available[next]) {
                select_imu(next);
                next = (next + 1) % 3;
                last_switch_ms = now_ms;
            }
        }

        // Service + read quaternion
        pico_bno08x_service(&active);

        sh2_SensorValue_t v;
        if (pico_bno08x_get_sensor_event(&active, &v) && v.sensorId == SH2_ROTATION_VECTOR) {
            quat[cur][0] = v.un.rotationVector.real;
            quat[cur][1] = v.un.rotationVector.i;
            quat[cur][2] = v.un.rotationVector.j;
            quat[cur][3] = v.un.rotationVector.k;

            float elapsed = (now_ms - t0_ms) / 1000.0f;

            // Print full .sto row (time + all 3 quats)
            printf("%.3f", elapsed);
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 4; j++) printf(",%.6f", quat[i][j]);
            }
            printf("\n");

            // Also send the *current* IMU update via UDP (time,label,w,x,y,z)
            if (wifi_ok) {
                (void)wireless_send_quat(elapsed, imus[cur].label,
                    quat[cur][0], quat[cur][1], quat[cur][2], quat[cur][3]);
            }
        }

        sleep_ms(POLL_DELAY_MS);
    }

    // Unreachable, but kept for completeness
    if (wifi_ok) wireless_deinit();
    return 0;
}