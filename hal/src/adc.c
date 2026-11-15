#include "hal/adc.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>

int adc_hal_init(adc_hal_t *adc, const char *device, uint32_t speed_hz) {
    uint8_t mode = 0;
    uint8_t bits = 8;

    if (adc == NULL || device == NULL) {
        return -1;
    }

    adc->fd = -1;
    adc->speed_hz = speed_hz;
    adc->fd = open(device, O_RDWR);
    if (adc->fd < 0) {
        perror("Failed to open SPI device");
        return -1;
    }
    if (ioctl(adc->fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("Failed to set SPI mode");
        adc_hal_cleanup(adc);
        return -1;
    }
    if (ioctl(adc->fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("Failed to set SPI bits per word");
        adc_hal_cleanup(adc);
        return -1;
    }
    if (ioctl(adc->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) == -1) {
        perror("Failed to set SPI speed");
        adc_hal_cleanup(adc);
        return -1;
    }

    return 0;
}

int adc_hal_read_channel(adc_hal_t *adc, uint8_t channel) {
    if (adc == NULL || adc->fd < 0) {
        return -1;
    }

    uint8_t tx[3] = { (uint8_t)(0x06 | ((channel & 0x04) >> 2)),
                      (uint8_t)((channel & 0x03) << 6),
                      0x00 };
    uint8_t rx[3] = {0};

    struct spi_ioc_transfer transfer = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = adc->speed_hz,
        .bits_per_word = 8,
        .cs_change = 0
    };

    if (ioctl(adc->fd, SPI_IOC_MESSAGE(1), &transfer) < 1) {
        return -1;
    }

    return ((rx[1] & 0x0F) << 8) | rx[2];
}

int adc_hal_cleanup(adc_hal_t *adc) {
    if (adc != NULL && adc->fd >= 0) {
        if (close(adc->fd) == -1) {
            perror("Failed to close SPI device");
            adc->fd = -1;
            return -1;
        }
        adc->fd = -1;
    }
    return 0;
}