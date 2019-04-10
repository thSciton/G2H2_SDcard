#ifndef PHYTECMODULE_H
#define PHYTECMODULE_H

#include <QObject>
#include <QDebug>
#include <QtSerialPort/QtSerialPort>

#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#define MAX_BUF 256
#define SYSFS_GPIO_DIR "/sys/class/gpio"

#define PHYTEC_BOOT_PIN 175
#define PHYTEC_RESET_PIN 42

#define PHYTEC_DEBUG_PORT "/dev/ttymxc4"


typedef enum
{
	NO_ERROR = 0,					// No Error, line written
	STATUS_NO_WRITE,				// No Error, NO line written
	STATUS_FW_SUCCESS,				// Final Firmware line OK
	ERR_FW_BAD_LINE,				// Firmware line has bad cookie
	ERR_FW_DECODE,					// Firmware line has bad record type
	ERR_FW_CHKSUM,					// Firmware line has bad checksum
	ERR_FW_VERIFY,					// Firmware verify error
} _GLOBAL_ERROR_CODES;

typedef enum {
	PIN_INPUT = 0,
	PIN_OUTPUT
} PIN_DIRECTION;

typedef enum {
	PIN_LOW = 0,
	PIN_HIGH
} PIN_VALUE;

class PhytecModule : public QObject
{
private:
	int sciton_sio_fd;
	struct termios tio;
	speed_t sioTtyRate;
	u_int8_t Current_Segment;
	u_int16_t line_nr;
    FILE *fw_file_ptr;
    int fw_file_size;

	void initSerial( void );
	void initGPIO( void );
	void initGPIOPin( u_int16_t pin_number, PIN_DIRECTION pin_dir, PIN_VALUE pin_val );
	void releaseGPIOPin( u_int16_t pin_number );
	void gpio_export(u_int16_t pin_number);
	void gpio_unexport(u_int16_t pin_number);
	void gpio_set_direction(u_int16_t pin_number, PIN_DIRECTION pin_dir );
	void gpio_set_value(u_int16_t pin_number, PIN_VALUE pin_val );
	int  gpio_get_value(u_int16_t pin_number );
	u_int8_t hex2nibble(u_int8_t c);
	u_int8_t hex2byte(u_int8_t *ptr);
	u_int8_t write_fw_hex_record(u_int8_t *record);
	u_int16_t hex2short(u_int8_t* ptr);
	void add_checksum(u_int8_t *buffer, u_int8_t len);
    int	read_fw_hex_line( char *line );
    int pollRx(int fd, int timeoutVal);

public:
	PhytecModule(const char* path);
	~PhytecModule();
    bool bIsFileOpened(void);
    u_int16_t sendSerial(const char *str);
    u_int16_t sendSerial(u_int8_t *str, int len);
	u_int16_t readSerial( char *buf, u_int16_t num_bytes );
	void setGPIOPin( u_int16_t pin_number );
	void resetGPIOPin( u_int16_t pin_number );
	void toggleGPIO( void );
    int connectModule( void );
    int updateModule( void );
    int initFWFile( const char* path );
    void releaseGPIO( void );
    void releaseFWFile( void );

};

#endif // PHYTECMODULE_H
