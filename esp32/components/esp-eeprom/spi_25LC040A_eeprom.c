#include <string.h>
#include <driver/spi_master.h>
#include "spi_25LC040A_eeprom.h"
#include "freertos/task.h"

// ESP32 Technical Reference Manual - p.122
// Table 7-2. Command Definitions Supported by GPSPI Slave in Halfduplex Mode
#define CMD_WRSR 0x01
#define CMD_WRITE 0x02
#define CMD_READ 0x03
#define CMD_WRDI 0x04
#define CMD_RDSR 0x05
#define CMD_WREN 0x06

#define PAGE_SIZE 16

esp_err_t spi_25LC040_init(spi_host_device_t masterHostId, int csPin, int sckPin, int mosiPin, int misoPin, int clkSpeedHz, spi_device_handle_t *pDevHandle)
{
    esp_err_t ret;

    spi_bus_config_t spiBusCfg = {
        .miso_io_num = misoPin,
        .mosi_io_num = mosiPin,
        .sclk_io_num = sckPin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    spi_device_interface_config_t masterCfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .clock_speed_hz = clkSpeedHz,
        .spics_io_num = csPin,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .queue_size = 1
    };

    ret = spi_bus_initialize(masterHostId, &spiBusCfg, SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_add_device(masterHostId, &masterCfg, pDevHandle);
    ESP_ERROR_CHECK(ret);

    return ret;
}

esp_err_t spi_25LC040_free(spi_host_device_t masterHostId, spi_device_handle_t devHandle)
{
    esp_err_t ret;

    ret = spi_bus_remove_device(devHandle);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_free(masterHostId);
    ESP_ERROR_CHECK(ret);

    return ret;

}

esp_err_t spi_25LC040_read_byte(spi_device_handle_t devHandle, uint16_t address, uint8_t *pData)
{
    esp_err_t ret;
    spi_transaction_t spiTrans;

    memset(&spiTrans, 0, sizeof(spiTrans));

    // 4-Kbit SPI Bus Serial EEPROM - p.8
    // FIGURE 3-1: READ SEQUENCE
    uint8_t read_seq[2];
    uint16_t addr_msb = address >> 8 & 0x01;
    read_seq[0] = CMD_READ | (addr_msb << 3);     // Instruction+Address MSb
    read_seq[1] = address;                        // Lower Address Byte
    spiTrans.length = sizeof(read_seq) * 8;
    spiTrans.tx_buffer = read_seq;
    spiTrans.rxlength = 8;
    spiTrans.rx_buffer = pData;                   // Data Out

    ret = spi_device_polling_transmit(devHandle, &spiTrans);
    assert(ret == ESP_OK);

    return ret;
}

esp_err_t spi_25LC040_write_byte(spi_device_handle_t devHandle, uint16_t address, uint8_t data)
{
    esp_err_t ret;
    spi_transaction_t spiTrans;

    memset(&spiTrans, 0, sizeof(spiTrans));

    // 4-Kbit SPI Bus Serial EEPROM - p.9
    // FIGURE 3-2: BYTE WRITE SEQUENCE
    uint8_t write_seq[3];
    uint16_t addr_msb = address >> 8 & 0x01;
    write_seq[0] = CMD_WRITE | (addr_msb << 3);    // Instruction+Address MSb
    write_seq[1] = address;                        // Lower Address Byte
    write_seq[2] = data;                           // Data Byte

    spiTrans.length = sizeof(write_seq) * 8;
    spiTrans.tx_buffer = write_seq;

    ret = spi_device_polling_transmit(devHandle, &spiTrans);
    assert(ret == ESP_OK);

    // Add a delay to allow time for the write operation to complete
    vTaskDelay(pdMS_TO_TICKS(10)); // Adjust the delay time as necessary

    return ret;
}

esp_err_t spi_25LC040_write_page(spi_device_handle_t devHandle, uint16_t address, const uint8_t* pBuffer, uint8_t size)
{
    if (size > PAGE_SIZE) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    spi_transaction_t spiTrans;

    memset(&spiTrans, 0, sizeof(spiTrans));

    // 4-Kbit SPI Bus Serial EEPROM - p.10
    // FIGURE 3-3: PAGE WRITE SEQUENCE
    uint8_t write_seq[2 + PAGE_SIZE];
    uint16_t addr_msb = address >> 8 & 0x01;
    write_seq[0] = CMD_WRITE | (addr_msb << 3);    // Instruction+Address MSb
    write_seq[1] = address;                        // Lower Address Byte
    memcpy(&write_seq[2], pBuffer, size);          // Data Bytes

    spiTrans.length = sizeof(write_seq) * 8;
    spiTrans.tx_buffer = write_seq;

    ret = spi_device_polling_transmit(devHandle, &spiTrans);
    assert(ret == ESP_OK);

    return ret;
}

esp_err_t spi_25LC040_write_enable(spi_device_handle_t devHandle)
{
    esp_err_t ret;
    spi_transaction_t spiTrans;

    memset(&spiTrans, 0, sizeof(spiTrans));

    // 4-Kbit SPI Bus Serial EEPROM - p.11
    // FIGURE 3-4: WRITE ENABLE SEQUENCE (WREN)
    uint8_t write_seq[1];
    write_seq[0] = CMD_WREN;            // Instruction

    spiTrans.length = sizeof(write_seq) * 8;
    spiTrans.tx_buffer = write_seq;

    ret = spi_device_polling_transmit(devHandle, &spiTrans);
    assert(ret == ESP_OK);

    return ret;
}

esp_err_t spi_25LC040_write_disable(spi_device_handle_t devHandle)
{
    esp_err_t ret;
    spi_transaction_t spiTrans;

    memset(&spiTrans, 0, sizeof(spiTrans));

    // 4-Kbit SPI Bus Serial EEPROM - p.11
    // FIGURE 3-5: WRITE DISABLE SEQUENCE (WRDI)
    uint8_t write_seq[1];
    write_seq[0] = CMD_WRDI;            // Instruction

    spiTrans.length = sizeof(write_seq) * 8;
    spiTrans.tx_buffer = write_seq;

    ret = spi_device_polling_transmit(devHandle, &spiTrans);
    assert(ret == ESP_OK);
    return ret;
}

esp_err_t spi_25LC040_read_status(spi_device_handle_t devHandle, uint8_t *pStatus)
{
    esp_err_t ret;
    spi_transaction_t spiTrans;

    memset(&spiTrans, 0, sizeof(spiTrans));

    // 4-Kbit SPI Bus Serial EEPROM - p.12
    // FIGURE 3-6: READ STATUS REGISTER TIMING SEQUENCE (RDSR)
    uint8_t read_seq[2];
    read_seq[0] = CMD_RDSR;                         // Instruction
    spiTrans.length = sizeof(read_seq) * 8;
    spiTrans.tx_buffer = read_seq;
    spiTrans.rxlength = 8;
    spiTrans.rx_buffer = pStatus;                   // Data Out

    ret = spi_device_polling_transmit(devHandle, &spiTrans);
    assert(ret == ESP_OK);

    return ret;
}

esp_err_t spi_25LC040_write_status(spi_device_handle_t devHandle, uint8_t status)
{
    esp_err_t ret;
    spi_transaction_t spiTrans;

    memset(&spiTrans, 0, sizeof(spiTrans));

    // 4-Kbit SPI Bus Serial EEPROM - p.13
    // FIGURE 3-7: WRITE STATUS REGISTER TIMING SEQUENCE (WRSR)
    uint8_t write_seq[2];
    write_seq[0] = CMD_WRSR;            // Instruction
    write_seq[1] = status;              // Status Byte

    spiTrans.length = sizeof(write_seq) * 8;
    spiTrans.tx_buffer = write_seq;

    ret = spi_device_polling_transmit(devHandle, &spiTrans);
    assert(ret == ESP_OK);

    return ret;
}