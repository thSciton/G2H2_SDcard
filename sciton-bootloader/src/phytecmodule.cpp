/**
  *****************************************************************************
  * @file phytecmodule.cpp
  * @date 11-January-2017
  * @brief Module to encapsulate Phytec Module functionality.
  *
  *****************************************************************************
  */

/*! \addtogroup BootLoader
  *  @{
  */
#include <termios.h>
#include "phytecmodule.h"
#include "bootcode.cpp"
#include "math.h"
#include <stdexcept>
#include <poll.h>

u_int16_t PhytecModule::hex2short(u_int8_t* ptr)
{
    u_int16_t b;

    b = hex2nibble(ptr[0]);
    b <<= 4;
    b |= hex2nibble(ptr[1]);
    b <<= 4;
    b |= hex2nibble(ptr[2]);
    b <<= 4;
    b |= hex2nibble(ptr[3]);
    return b;
}

u_int8_t PhytecModule::hex2nibble(u_int8_t c)
{
	if (c>0x60) c-= 0x20; 	// convert to upper case
							// numbers are 0x30-0x39
							// ascii letters are 0x41-0x5? or 0x61-0x7?
	if (c>='0' && c<='9') 
		return (c-'0');
	else 
		return (c - 'A'+ 0x0A) ;
}

u_int8_t PhytecModule::hex2byte(u_int8_t *ptr)
{
    u_int8_t b;
	b = hex2nibble(ptr[1]) + (hex2nibble(ptr[0])<<4);
	return b;
}

void PhytecModule::add_checksum(u_int8_t *buffer, u_int8_t len)
{
	u_int8_t i;
	int8_t checksum = buffer[0];

	for(i=1; i<len; i++)
		checksum += buffer[i];
	buffer[len] = (u_int8_t)(-checksum);
}

int	PhytecModule::read_fw_hex_line( char *line )
{
    char *ln = NULL;
    size_t len = 0;
	ssize_t read = -1;

	if (fw_file_ptr != NULL)
	{
        read = getline(&ln, &len, fw_file_ptr);
        strcpy(line,ln);
        if(ln){free(ln);}
	}
	else
	{
        qDebug("PhytecModule::Could not open file\n");
	}

	return read;
}

u_int8_t PhytecModule::write_fw_hex_record(u_int8_t *record)
{
    u_int8_t i, chksum, status = NO_ERROR;	// Assume OK, line will be written
    u_int8_t len, record_type;
    u_int8_t line[22];
    u_int16_t offset;

    // Record Format:
    //
    // +--------+--------+------+-------+--------+------(n bytes)------+----------+
    // | RECORD | RECLEN |    OFFSET    | RECORD |                     | CHECKSUM |
    // |  MARK  |  (n)   |   (2 BYTES)  |  TYPE  |        DATA         |          |
    // |  ':'   |        |              |        |                     |          |
    // +--------+--------+------+-------+--------+------(n bytes)------+----------+
    //
    if(record[0] == ':')
    {
        chksum = len = hex2byte(record + 1);	// get the record length, initialize checksum
        offset = hex2short(record + 3);  		// get the starting address (offset field in HEX record)
        record_type = hex2byte(record + 7);		// get the record type
        // only 0, 1 and 4 are valid types
        if((record_type == 0) || (record_type == 1) || (record_type == 4))
        {
        	for(i=2; i<len + 6; i++)
                chksum += line[i] = hex2byte(record + (2*i - 1));	// check if checksum is OK
            if(chksum)
                    status = ERR_FW_CHKSUM;			// checksum should be 0
            else									// checksum is OK
            {
                if(record_type == 1)				// record type '1' is final record
                    status = STATUS_FW_SUCCESS;		// indicate success
                else if	(record_type == 4)			// record type '4'
                {
                	if((len != 2) || (offset != 0))
                        status = ERR_FW_DECODE;		// Bad record type
                    else
                    {
                    	Current_Segment = line[6];
                        status = STATUS_NO_WRITE;	// NO Line Written
                    }
                }
                else								// record type '0'
                {
                    // Re-arrange the line a bit to make up a Phytec command
                    // Offset and Data stay in place
                    line[0] = 0x0B;					// Write command
                    line[1] = Current_Segment;
                    line[4] = len;					// replace the offset with length byte
                    if(len < 17)
                    {
                        add_checksum(line, len + 5);	// Add Checksum to command
                        sendSerial(line, len + 6);	// Write the command
                    }
                    else
                    status = ERR_FW_DECODE;
                line_nr++;
            }
        }
    }
    else
            status = ERR_FW_DECODE;					// Bad record type
    }
    else
        status = ERR_FW_BAD_LINE;					// ':' missing from record

    return status;
}

/**
 * @brief      Export sysfs gpio pin
 *
 * @param[in]  pin_number  The pin number
 */
void PhytecModule::gpio_export(u_int16_t pin_number)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	len = snprintf(buf, sizeof(buf), "%d", pin_number );
	write(fd, buf, len);
	close(fd);
}

/**
 * @brief      Unexport sysfs gpio pin
 *
 * @param[in]  pin_number  The pin number
 */
void PhytecModule::gpio_unexport(u_int16_t pin_number)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	len = snprintf(buf, sizeof(buf), "%d", pin_number );
	write(fd, buf, len);
	close(fd);
}

/**
 * @brief      Set sysfs pin direction.
 *
 * @param[in]  pin_number  The pin number
 * @param[in]  pin_dir     The pin dir
 */
void PhytecModule::gpio_set_direction(u_int16_t pin_number, PIN_DIRECTION pin_dir )
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), "%s/gpio%d/direction", SYSFS_GPIO_DIR, pin_number );
	fd = open(buf, O_WRONLY);

	if (pin_dir == PIN_OUTPUT)
	{
		len = snprintf(buf, sizeof(buf), "%s", "out" );
	}
	else
	{
		len = snprintf(buf, sizeof(buf), "%s", "in" );
	}

	write(fd, buf, len);
	close(fd);
}

/**
 * @brief      Set sysfs pin value.
 *
 * @param[in]  pin_number  The pin number
 * @param[in]  pin_val     The pin value
 */
void PhytecModule::gpio_set_value(u_int16_t pin_number, PIN_VALUE pin_val )
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), "%s/gpio%d/value", SYSFS_GPIO_DIR, pin_number );
	fd = open(buf, O_WRONLY);

	if (pin_val == PIN_HIGH)
	{
		len = snprintf(buf, sizeof(buf), "%d", 1 );
	}
	else
	{
		len = snprintf(buf, sizeof(buf), "%d", 0 );
	}

	write(fd, buf, len);
	close(fd);
}

/**
 * @brief      Get sysfs pin value.  Pin directinon must be an input.
 *
 * @param[in]  pin_number  The pin number
 *
 * @return     Current value of the pin.
 */
int PhytecModule::gpio_get_value(u_int16_t pin_number )
{
	char pin_val;
	int fd, result;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), "%s/gpio%d/value", SYSFS_GPIO_DIR, pin_number );
	fd = open(buf, O_RDONLY);
	read(fd, &pin_val, 1);

	if (pin_val == '0')
	{
		result = 0;
	}
	else
	{
		result = 1;
	}

	close(fd);

	return result;
}

/**
 * @brief      Initialize the serial port.  This acquire a termios file
 *             descriptor and opens the port as read write.  The purpose of
 *             this port is to send commands to the phytec module, and to receive
 *             acknowledgements.
 */
void PhytecModule::initSerial( void )
{
	sciton_sio_fd = -1;
    sioTtyRate =  B230400;

	memset(&tio, 0, sizeof(tio));
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag = CS8 | CREAD | CLOCAL;
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 5;

	sciton_sio_fd = posix_openpt(O_RDWR | O_NOCTTY);
	tcsetattr(sciton_sio_fd, TCSANOW, &tio);

	grantpt(sciton_sio_fd);
	unlockpt(sciton_sio_fd);

    sciton_sio_fd = open(PHYTEC_DEBUG_PORT, O_RDWR);

    //qDebug() << "sciton_sio_fd == " << sciton_sio_fd << "\n";

	cfsetospeed(&tio, sioTtyRate);
	cfsetispeed(&tio, sioTtyRate);
	tcsetattr(sciton_sio_fd, TCSANOW, &tio);
}

/**
 * @brief      Send a message to the Phytech module's serial port.
 *
 * @param[in]  str   The message to be sent.
 */
u_int16_t PhytecModule::sendSerial(const char *str)
{
	char   buf[MAX_BUF];
    size_t len = -1, wrote = -1;

    if(sciton_sio_fd > -1 )
    {	
    	sprintf(buf, "%s\r\n", str);
    	len = strlen(buf);

    	//qDebug() << "sciton_sio_fd == " << sciton_sio_fd << "length = " << len << "\n";

    	wrote = write(sciton_sio_fd, buf, len );

    	//qDebug() << "buf = " << buf << "\n";
    	//qDebug() << "wrote = " << wrote << "\n";
    }
    else
    {
    	qDebug() << "Serial Port has not been initialized.\n";
    }

    return wrote;
}

u_int16_t PhytecModule::sendSerial( u_int8_t *str, int len)
{
    size_t wrote = -1;

    if(sciton_sio_fd > -1 )
    {	
    	wrote = write(sciton_sio_fd, str, len );
    	//qDebug() << "wrote = " << wrote << "\n";
    }
    else
    {
    	qDebug() << "Serial Port has not been initialized.\n";
    }

    return wrote;
}

/**
 * @brief      Read the Phytec module's serial port.
 *
 * @param      buf        The buffer to receive the data into.
 * @param[in]  num_bytes  The number bytes to read
 *
 * @return     The number of bytes read, or -1 in case of failure.
 */
u_int16_t PhytecModule::readSerial( char *buf, u_int16_t num_bytes )
{
    size_t len = -1;

    if(sciton_sio_fd > -1 )
    {	
    	len 	= read(sciton_sio_fd, buf, num_bytes );
    	//qDebug("read buf = %x\n", buf[0]);
    	//qDebug() << "read len = " << len << "\n";
    }
    else
    {
    	qDebug() << "Serial Port has not been initialized.\n";
    }

    return len;
}

/**
 * @brief      Initialize a GPIO pin.  
 *
 * @param[in]  pin_number  The pin number
 * @param[in]  pin_dir     The pin direction
 * @param[in]  pin_val     The pin initial value
 */
void PhytecModule::initGPIOPin( u_int16_t pin_number, PIN_DIRECTION pin_dir, PIN_VALUE pin_val )
{
	gpio_export(pin_number);
	gpio_set_direction(pin_number, pin_dir);
    gpio_set_value(pin_number, pin_val);
}

/**
 * @brief      Sets the gpio pin to logic high.
 *
 * @param[in]  pin_number  The pin number
 */
void PhytecModule::setGPIOPin( u_int16_t pin_number )
{
    gpio_set_value(pin_number, PIN_HIGH);
}

/**
 * @brief      Resets the gpio pin to logic low.
 *
 * @param[in]  pin_number  The pin number
 */
void PhytecModule::resetGPIOPin( u_int16_t pin_number )
{
    gpio_set_value(pin_number, PIN_LOW);
}

/**
 * @brief      Releases the gpio pin so that it can be used by
 *             a driver.
 *
 * @param[in]  pin_number  The pin number
 */
void PhytecModule::releaseGPIOPin( u_int16_t pin_number )
{
	gpio_unexport(pin_number);
    qDebug(" PhytecModule::releaseGPIOPin\n");
}

/**
 * @brief      Initialize all phytec module gpio pins.
 */
void PhytecModule::initGPIO( void )
{
	initGPIOPin(PHYTEC_BOOT_PIN, PIN_OUTPUT, PIN_HIGH);
	initGPIOPin(PHYTEC_RESET_PIN, PIN_OUTPUT, PIN_LOW);
}

/**
 * @brief      Convenience function to toggle gpio.
 */
void PhytecModule::toggleGPIO( void )
{
	static bool tog = true;

	if(tog)
	{
		setGPIOPin(PHYTEC_BOOT_PIN);
		resetGPIOPin(PHYTEC_RESET_PIN);
		tog = false;
	}
	else
	{
		resetGPIOPin(PHYTEC_BOOT_PIN);
		setGPIOPin(PHYTEC_RESET_PIN);
		tog = true;
	}
}

/**
 * @brief      Release all phytec module gpio pins.
 */
void PhytecModule::releaseGPIO( void )
{
    resetGPIOPin(PHYTEC_BOOT_PIN);
    resetGPIOPin(PHYTEC_RESET_PIN);
    QThread::msleep( 1000 );
    setGPIOPin(PHYTEC_RESET_PIN);

    //releaseGPIOPin(PHYTEC_BOOT_PIN);
    //releaseGPIOPin(PHYTEC_RESET_PIN);
}

/**
 * @brief      Establish a connection with the phytec module, and upload the
 *             bootcode.
 */
int PhytecModule::connectModule( void )
{
    int ret, rd_status = -1;
    char   buf[MAX_BUF];
    u_int8_t command[] = {0x00};
    int fd = sciton_sio_fd;
    // Connect State 0, reset low, boot high
	setGPIOPin(PHYTEC_BOOT_PIN);
	resetGPIOPin(PHYTEC_RESET_PIN);

	// Wait 10 ms, raise RESET, keep BOOT high
    QThread::msleep( 10 );
	setGPIOPin(PHYTEC_RESET_PIN);

    //-------------------- phase 1 -------------------
    QThread::msleep( 500 );
    qDebug(" connectModule: Waited 500 ms for phytec to wake up, now send first test command '00'");
    write( sciton_sio_fd, command, 1 );

    qDebug(" connectModule: now wait 100 msec for reponse D5h");
    ret = -1;
	// Wait 100 ms for a phytec response
    QThread::msleep( 100 );

    rd_status = pollRx(fd, 2000);    // 2000 msec
    if (rd_status == 1) // 1: incoming data is available, 0: no data at time out
        ret = readSerial(buf,64);   // good response in buf[0] is D5h, else some value
    else {
        qDebug(" phase 1. first read time out");
        return -1;
    }
    qDebug(" connectModule: first reponse to command 00 is: %0X", buf[0]);  // D5h
    if ((int)(buf[0]) != 0xD5) {
        qDebug(" phase 1. response to command 00 is invalid (%0X). Abort connection.\n", buf[0]);
        return -2;
    }

    //-------------------- phase 2 -------------------
    qDebug(" connectModule: phase 2. now send level 1 boot code");
    write( sciton_sio_fd, boot_code, 32 );
    // Wait 100 ms for a phytec response
    QThread::msleep( 100 );

    qDebug(" connectModule: waited 100 msec, and now expect reponse 31h");

    rd_status = pollRx(fd, 2000);    // 2000 msec
    if (rd_status == 1) // 1: incoming data is available, 0: no data at time out
        ret = readSerial(buf,1);   // good response in buf[0] is 31h, else some value
    else {
        qDebug(" phase 2. 2nd read time out\n");
        return -1;
    }
    // check if the Host returns at least 1 (number of bytes)
    if (ret < 1) {
        qDebug(" connectModule: phase 2. No response from serial port");
        return ret;
    }
    // The Phytec CPU should response 0x31 to acknowledge the boot code
    qDebug(" connectModule: phase 2. reponse 1: %0X", buf[0]);
    if(0x31 != buf[0]) {
        qDebug(" connectModule: phase 2. level 1 bootcode response (%0X) not valid", buf[0]);
        return (int)(buf[0]);
    }

    //-------------------- phase 3 -------------------
    int repeat_count = (sizeof(boot_code)/32);
    qDebug(" phase 3. send number of bytes: %d", repeat_count-1);
    for(int x = 1; x < repeat_count; x++)
    {
        write( sciton_sio_fd, boot_code+(32*x), 32 );
    }

    // Wait 500 ms for phytec to wake up, send first test command byte '00'
    QThread::msleep( 500 );
    write( sciton_sio_fd, command, 1 );

    // Wait 100 ms for a phytec response then read
    QThread::msleep( 100 );

    // The Phytec CPU should response 0x01 to acknowledge       
    rd_status = pollRx(fd, 2000);   // 2000 msec
    if (rd_status == 1) // 1: incoming data is available, 0: no data at time out
        ret = readSerial(buf,1);   // good response in buf[0] is 1, else some value
    else {
        qDebug(" phase 3. 3rd read time out");
        return -1;
    }
    qDebug(" connectModule: phase 3. final reponse 2: %0X", buf[0]);

    if (1 != (int)(buf[0]))
        return -1;

    return 0;   // 0: success connection, boot code is ready. It's time to do update
}

/**
 * @brief      Erase the firmware, and update the module.
 */
int PhytecModule::updateModule( void )
{
	u_int8_t status = NO_ERROR;
    static char   buf[MAX_BUF];
    static char   record[MAX_BUF];
	float percent_done = (float)0.0;
	int bytes_remaining = fw_file_size;
    int rd_status = -1;
    int fd = sciton_sio_fd;
    qDebug(" updateModule: Erasing Flash");

    u_int8_t command[] = {0x09, 0xF7};
    write( sciton_sio_fd, command , 2 );

    qDebug(" updateModule: Waiting for Erase Completion...");

    QThread::msleep( 30000 );
    rd_status = pollRx(fd, 2000);
    if (rd_status == 1) // 1: incoming data is available, 0: no data at time out
        readSerial(buf,3);   // good response in buf[2] is 0x06, else some value
    else {
        qDebug(" updateModule: Eraser read time out");
        return -5;
    }

    if(buf[2]==0x06)
    {
        qDebug("Flash Erase Completed.\n");
        qDebug("buf = %x, %x, %x\n" ,buf[0], buf[1],buf[2]);
        qDebug("Read FW Hex File.\n");
        qDebug(" percent done: %2.2f", percent_done);
        qDebug(" bytes_remaining: %d", bytes_remaining);

        while( read_fw_hex_line(record) > 0 )
        {
            //qDebug("Line Retrieved: %s\n", record);
            //qDebug("Write line to Module.\n");
            status = write_fw_hex_record((u_int8_t *)record);
            QThread::msleep( 3 );   // this value achieve download in 6 minutes (from 20 minutes)

            bytes_remaining-=1;
			percent_done = 100.0 * ((float)bytes_remaining)/((float)fw_file_size);
            qDebug() << "\033[2K" << "Percent Remaining: " << qSetRealNumberPrecision(3)
                     << percent_done << "%";

        	if(status != NO_ERROR)
        	{
                qDebug("writing to Module. status:%0X\n", status);
        	}
        };

        qDebug("FW Hex File Read Completed.");
    }
    else {
        status = -6;
        qDebug(" Flash Erase NOT Completed. Code: %0X\n", buf[2]);
    }
    if (status == 2)    // writer state machine returns 2 on success
        status = 0;

    return status;
}

int countLinesInFile( FILE * fp )
{
	unsigned int number_of_lines = 0;
    int ch;

    while (EOF != (ch=getc(fp)))
	{
		if ('\n' == ch)
        ++number_of_lines;
	}

    return number_of_lines;
}

int PhytecModule::initFWFile( const char* path  )
{
    int result = -1;

	fw_file_ptr = fopen( path , "r");

	if (fw_file_ptr != NULL)
	{
		fw_file_size = countLinesInFile(fw_file_ptr);
		rewind(fw_file_ptr);
        result = 0;
        qDebug ("FW File Open for Reading. ( size = %d )", fw_file_size );
	}
	else
	{
        qDebug ("Could not open FW file.\n");
	}
    return result;
}

bool PhytecModule::bIsFileOpened(void)
{
    if (fw_file_ptr != NULL)
        return  true;
    return false;
}

void PhytecModule::releaseFWFile( void )
{
	if (fw_file_ptr != NULL)
	{
		fclose(fw_file_ptr);
	}
    qDebug(" PhytecModule::releaseFWFile\n");
}

/**
 * @brief      Phytec Module Constructor.  Initialize GPIO and serial port.
 */
PhytecModule::PhytecModule(const char* path)
{
    // Initialize GPIO
    initGPIO();
    // Initialize Serial Port
    initSerial();
	// Initialize FW File
    if( initFWFile( path ) != 0 )
    {
         throw std::runtime_error( "Invalid FW Path" );
    }
}

/**
 * @brief      Phytec Module Destructor.  Release all resources.
 */
PhytecModule::~PhytecModule()
{
	// Release GPIO
	releaseGPIO();	
	// Release FW File
	releaseFWFile();
}

/**
 * @brief      poll the serial port input
 */
int PhytecModule::pollRx(int fd, int timeoutVal)
{
    int rd_status = -1;
    struct pollfd poll_struct = {fd, POLLIN, 0};
    rd_status = poll(&poll_struct, 1, timeoutVal);
    return rd_status;
}

