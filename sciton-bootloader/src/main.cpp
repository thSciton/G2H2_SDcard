/**
  *****************************************************************************
  * @file main.cpp
  * @date 11-January-2017
  * @brief Entry point to sciton_bootloader application
  *
  *****************************************************************************
  */

/*! \addtogroup BootLoader
  *  @{
  */

// revision 2

#include <QtCore/QCoreApplication>
#include <QTimer>
#include <QTextStream>
#include <QDebug>
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <QThread>
#include <phytecmodule.h>
#include <stdexcept>

#include <sys/time.h>		/* for setitimer */
#include <signal.h>		/* for signal */


#define SCITON_BOOT_LOADER_NAME "sciton_bootloader"
#define EXECUTABLE_ARG 0
#define FW_PATH_ARG 1
#define REQUIRED_ARG_NUM 2

void usage(const char* msg)
{
    qDebug() << "Usage: " << SCITON_BOOT_LOADER_NAME << "FWPATH";
	qDebug() << msg;
}

void errorMsg(const char* msg)
{
    qDebug() << "Error: " << SCITON_BOOT_LOADER_NAME << " - " << msg;
}

struct itimerval it_val;	/* for setting itimer */
void timeoutCallback(void);
int timerInit(unsigned int interval);

int ret = -1;
int init = 0;

PhytecModule* ptec;

/**
 * @brief      Entry point to Sciton Bootloader
 *
 * @param[in]  argc  The argc
 * @param      argv  The argv
 *
 * @return     None
 */
int main(int argc, char *argv[])
{    
    if(argc == REQUIRED_ARG_NUM)
	{
	    qDebug() << "Creating Phytec Module";
        try
        {
            ptec = new PhytecModule(argv[FW_PATH_ARG]);
            if (!ptec->bIsFileOpened())
            {
                qDebug(" File open error. Abort");
                delete ptec;
                return -1;
            }

            init = timerInit(400);    // about 6+ minutes

            qDebug() << "Connect to the Module";
            ret = ptec->connectModule();
            if (ret < 0)
            {   // -1: polling timeout. -2: no data or invalid data received
                qDebug() << "main: no connection response from the Host.\n";
                delete ptec;
                return ret;
            }

            if (0x31 == ret)
            {
                delete ptec;
                qDebug() << " conenction: invalid level 1 reponse. Abort.\n";
                return ret;
            }

            // if the connection was established within max. 5 second then continue
            ret = -1;
            qDebug() << " Update the Module\n";
            ret = ptec->updateModule();
            delete ptec;
        }
        catch(std::runtime_error &e)
        {
            std::cout << "Could not create the Phytec Module\n";
        }

	}
	else
	{
        usage("Incorrect number of arguments\n");
	}
    //return sciton_app.exec();
    return ret;   // 0: success
}

void timeoutCallback(void)
{
    if (init < 0)
        return;

    qDebug() << " Time out in update process! Exit.";
    if (ptec)
        delete ptec;
    exit(-4);
}

int timerInit(unsigned int interval)
{
    // Upon SIGALRM, call timeoutCallback().
    if (signal(SIGALRM, (void (*)(int)) timeoutCallback) == SIG_ERR) {
      perror("Unable to catch SIGALRM");
      qDebug() << " Unable to catch SIGALRM.";
      return -1;
    }
    it_val.it_interval.tv_sec =     0;
    it_val.it_interval.tv_usec =    0;
    it_val.it_value.tv_sec = interval; /* set timer for 'INTERVAL' */
    it_val.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
      perror(" error calling setitimer()");
      qDebug() << " Terror calling setitimer().";
      return -1;
    }
    qDebug() << " Timer is set to go ...";
    return 0;
}


/*! @} */
