#include "spi_flash.h"

/*
 * Global variables
 */
unsigned char static_buffer[BUFFER_MAX_TOTAL_SIZE];

/*
 * Generic functions
 */
void print_buffer(unsigned char *buf, int count)
{
        int i = 0;
        for (; i < count; i++) {
                printf("%02x", buf[i]);
                if ((i % 16) == 7)
                        printf("  ");
                else if ((i % 16) == 15)
                        printf("\n");
                else
                        printf(" ");
        }
        if ((i % 16) != 0) printf("\n");
}

int spi_init()
{
        int fd;

        printf("Initializing...\n");
        fd = wiringPiSPISetupMode(SPI_CHANNEL, SPI_SPEED, SPI_MODE);
        //sleep(1);

        printf("Done initialized.\n");
        printf("SPI channel: %d\n", SPI_CHANNEL);
        printf("SPI speed: %d\n", SPI_SPEED);
        printf("SPI mode: %d\n", SPI_MODE);
        printf("\n");

        return fd;
}

int spi_close(int fd)
{
        int ret;
        ret = close(fd);
        return ret;
}

int spi_erase_sector(int addr)
{
        int ret = 0;
        int buf_size = BUFFER_RESERVED_BYTE;               // buffer size for erase operation always is contant

        unsigned char *buf = calloc(buf_size, sizeof(*buf));
        if (buf == NULL) {
                perror("Realloc error:");
                exit(5);
        }

        //Populate buffer to send
        buf[0] = SECTOR_ERASE;
        //taking little endian into account
        buf[1] = *((char*)&addr + 2);
        buf[2] = *((char*)&addr + 1);
        buf[3] = *(char*)&addr;

        printf("Erasing a sector of 4 KiB at address %06x ...\n", addr);
        spi_write_enable();
        ret = wiringPiSPIDataRW(SPI_CHANNEL, buf, buf_size);
        //TODO: poll BUSY bit until erase operation is finished
        printf("Finish erasing!\n");

        free(buf);
        return ret;
}

int spi_erase_block(int addr)
{
        int ret = 0;
        int buf_size = BUFFER_RESERVED_BYTE;               // buffer size for erase operation always is contant

        unsigned char *buf = calloc(buf_size, sizeof(*buf));
        if (buf == NULL) {
                perror("Realloc error:");
                exit(5);
        }

        //Populate buffer to send
        buf[0] = BLOCK_ERASE;
        //taking little endian into account
        buf[1] = *((char*)&addr + 2);
        buf[2] = *((char*)&addr + 1);
        buf[3] = *(char*)&addr;

        printf("Erasing a block of 64 KiB at address %06x ...\n", addr);
        spi_write_enable();
        ret = wiringPiSPIDataRW(SPI_CHANNEL, buf, buf_size);
        //TODO: poll BUSY bit until erase operation is finished
        printf("Finish erasing!\n");

        free(buf);
        return ret;
}

int spi_erase_chip(void)
{
        int ret = 0;

        unsigned char buf = CHIP_ERASE;

        printf("Erasing the whole chip (8 MiB) ...\n");
        spi_write_enable();
        ret = wiringPiSPIDataRW(SPI_CHANNEL, &buf, 1);
        //TODO: poll BUSY bit until erase operation is finished
        printf("Finish erasing!\n");

        return ret;
}

int spi_read_data(int addr, unsigned char *buf, int count)
{
        int ret = 0;
        int i = 0 ;
        int tr_size = 0;

        while (count > 0) {
                //Populate buffer to send
                static_buffer[0] = READ_DATA;
                //taking little endian into account
                static_buffer[1] = *((char*)&addr + 2);
                static_buffer[2] = *((char*)&addr + 1);
                static_buffer[3] = *(char*)&addr;
                tr_size = count > BUFFER_MAX_DATA_SIZE ?
                        BUFFER_MAX_TOTAL_SIZE :
                        (count + BUFFER_RESERVED_BYTE);
                ret = wiringPiSPIDataRW(SPI_CHANNEL, static_buffer, tr_size);
                memcpy(buf + i*BUFFER_MAX_DATA_SIZE,
                                static_buffer + BUFFER_RESERVED_BYTE,
                                tr_size - BUFFER_RESERVED_BYTE);
                count -= tr_size - BUFFER_RESERVED_BYTE;
                addr += tr_size - BUFFER_RESERVED_BYTE;
                i++;
        }

#ifdef VERBOSE
        printf("Read data return: %d\n", ret);
#endif
        return ret;
}

int spi_write_enable()
{
        int ret = 0;
        unsigned char buf = WRITE_ENABLE;
        ret = wiringPiSPIDataRW(SPI_CHANNEL, &buf, 1);
#ifdef VERBOSE
        printf("Write enable return: %d\n", ret);
#endif
        return ret;
}

int spi_write_disable()
{
        int ret = 0;
        unsigned char buf = WRITE_DISABLE;
        ret = wiringPiSPIDataRW(SPI_CHANNEL, &buf, 1);
#ifdef VERBOSE
        printf("Write disable return: %d\n", ret);
#endif
        return ret;
}

int spi_write_data(int addr, unsigned char *buf, int count)
{
        int ret = 0;
        int start_addr = addr;
        int tr_size = 0;
        int partial_page_size = 0;

        while (count > 0) {
                //Populate buffer to send
                static_buffer[0] = PAGE_PROGRAM;
                //taking little endian into account
                static_buffer[1] = *((char*)&addr + 2);
                static_buffer[2] = *((char*)&addr + 1);
                static_buffer[3] = *(char*)&addr;
                partial_page_size = PAGE_SIZE - (addr & 0x0000FF);
                tr_size = count > partial_page_size ?
                        (partial_page_size + BUFFER_RESERVED_BYTE) :
                        (count + BUFFER_RESERVED_BYTE);
                memcpy(static_buffer + BUFFER_RESERVED_BYTE,
                                buf + (addr - start_addr),
                                tr_size - BUFFER_RESERVED_BYTE);
                spi_write_enable();
                ret = wiringPiSPIDataRW(SPI_CHANNEL, static_buffer, tr_size);
                // TODO polling until BUSY bit is cleared
                sleep(1);
                count -= tr_size - BUFFER_RESERVED_BYTE;
                addr += tr_size - BUFFER_RESERVED_BYTE;
        }

#ifdef VERBOSE
        printf("Write data return: %d\n", ret);
#endif
        return ret;
}

int spi_read_BUSY_bit(void)
{
        int busy;
        unsigned char buf[2];
        //Populate buffer to send
        buf[0] = READ_STATUS_REGISTER_1;
        wiringPiSPIDataRW(SPI_CHANNEL, buf, 2);
        busy = buf[1] & BUSY_BIT_MASK;
        return busy;
}

void dump_flash(const char *name)
{
        int ret = 0;
        FILE *dump_file = fopen(name, "w");
        if (dump_file == NULL){
                perror("Open dump_file error:");
                exit(1);
        }

        int transaction_count = MAIN_FLASH_SIZE / BUFFER_MAX_DATA_SIZE;
        int data_buffer_size = BUFFER_MAX_DATA_SIZE;       /* 2048 bytes */
        int total_buffer_size = BUFFER_MAX_TOTAL_SIZE;         /* 2052 bytes */
        printf("Data buffer size is: %d\n", data_buffer_size);
        printf("Total buffer size is: %d\n", total_buffer_size);

        printf("Dumping whole flash to file [%s] ...\n", name);

        int addr = 0;
        for (int i = 0; i < transaction_count; i++) {
                addr = i * data_buffer_size;

                //Populate buffer to send
                static_buffer[0] = READ_DATA;
                //taking little endian into account
                static_buffer[1] = *((char*)&addr + 2);
                static_buffer[2] = *((char*)&addr + 1);
                static_buffer[3] = *(char*)&addr;

                ret = wiringPiSPIDataRW(SPI_CHANNEL, static_buffer, total_buffer_size);
                if (ret != total_buffer_size) {
                        printf("SPIDataRW failure!\n");
                        exit(1);
                }
                fwrite(static_buffer + BUFFER_RESERVED_BYTE, sizeof(char), data_buffer_size, dump_file);
        }

        printf("Finish dumping!\n");
        fclose(dump_file);
}
