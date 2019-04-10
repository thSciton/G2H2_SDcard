#!/bin/bash -x
# -x option is for showing the command to be run
# sciton_update_v6.sh
# use updated qml-viewer application (add sync command for write op)
export TMPDIR=/mnt/.psplash

SN="##  Ver. 46_02  rev. 2.01    Welcome!  ##"                                                                            
/usr/bin/psplash-write "HDR $SN"

/usr/bin/psplash-write "MSG Detecting USB drive, please wait for 5 seconds ..."
# wait so the slow-type usb drive come up and seen by the Linux
sleep 5
       
USB_DRIVE_PATH='/run/media/'
REACH_DISPLAY_FILE_NAME='sciton_reach_update.tgz'
REACH_DISPLAY_FILE="$(ls /run/media/ | grep 'sd*')"

REVFILE='/run/media/'${REACH_DISPLAY_FILE}/'UI_rev'
SYS_FILE_NAME='sys.sh'
SYS_FILE='/run/media/'${REACH_DISPLAY_FILE}/'sys.sh'
	
SCITON_FW_IMG_EXT="$(find $USB_DRIVE_PATH -name '*.H86')"
REACH_DISPLAY_PATH=${USB_DRIVE_PATH}${REACH_DISPLAY_FILE}/${REACH_DISPLAY_FILE_NAME}

APPLICATION_PATH='/application/'
SCITON_FW_UPDATER=${APPLICATION_PATH}'bin/sciton-bootloader'
PRINT_FUNC='/usr/bin/psplash-write'
PROGRESS_FUNC='/usr/bin/psplash-write'

g2h2_cmd_status=-1
joule_cmd_status=-1
errrorcode=0

G2H2_FILESTAT=FFFFFFFF
display_updated=0
phytec_updated=0

files_exist=0
currentRev=""
qmlRev=""
RT=$'\r'
/usr/bin/psplash-write "MSG Start!"
echo "0" > /home/errorlog

# At first, save a copy system settings regardless USB-drive existing nor not
# this is an attempt to restore the settings if the file is corrupted
ret=""
ret="`grep -F -w "Application" /application/src/application.conf`"
if [[ $ret != "[Application]" ]]; then
    echo "Restore the saved file to the src folder"
    /usr/bin/psplash-write "MSG Restore settings ..."
    cp /application/application.conf /application/src/.
    sync
    chmod 777 /application/src/application.conf
    sync
    aplay "/application/src/sounds/clicksweeper.wav"
    sleep 1s
else
    echo "Save the settings to a backup file ..."
    /usr/bin/psplash-write "MSG save settings ..."
    cp /application/src/application.conf /application/.
    sync
    chmod 777 /application/src/application.conf
    sync
    sleep 2s
fi

function g2h2Progress() {
	for i in {0..99}
		do
			/usr/bin/psplash-write "PROGRESS ${i}"
			/usr/bin/psplash-write "MSG Display Module Software update ${i} %"
			usleep 550
		done
}

function jouleProgress() {
    /usr/bin/psplash-write "MSG Connecting to joule System ... "
    joule_cmd_status=-1
    sleep 4s
	for i in {0..99}
	do
                    #joule_cmd_status=$(cat /home/errorlog)
        joule_cmd_status="$(grep '[0-9]' /home/errorlog)"
        if (( joule_cmd_status != 0)); then
            /usr/bin/psplash-write "MSG joule System Software not updated. error #${joule_cmd_status}. Abort!"         
            sleep 1s
                #if test $joule_cmd_status -gt 0; then
            if (( joule_cmd_status > 200)); then
                /usr/bin/psplash-write "MSG joule connection error #${status}. Abort!"                                                                    
            fi
            sleep 2s
            return -1
        else
        	usleep 800000
			/usr/bin/psplash-write "PROGRESS ${i}"
            usleep 800000
			/usr/bin/psplash-write "MSG joule System Software update ${i} %"
			usleep 800000
			/usr/bin/psplash-write "MSG joule System Software update ${i}.5 %"          
        fi            
	done
    /usr/bin/psplash-write "MSG One moment please ... "
    sleep 2s
    /usr/bin/psplash-write "MSG Finalizing update process ... "
    sleep 2s
}

function updatePhytecModule() {
    if [ -f ${SCITON_FW_IMG_EXT} ]; then
    	/usr/bin/psplash-write "MSG Updating joule System Software (about 6 minutes) ..."
    	/usr/bin/psplash-write "PROGRESS 20"
        jouleProgress &
    	${SCITON_FW_UPDATER} ${SCITON_FW_IMG_EXT}
        errrorcode=$?
        echo $errrorcode > /home/errorlog  
        if [[ errrorcode -ne 0 ]];  then
            phytec_updated=0 
            sleep 3s
            return -1
        else
            joule_cmd_status=0
            /usr/bin/psplash-write "PROGRESS 100"
            phytec_updated=1
        fi
        /usr/bin/psplash-write "MSG One moment please ... "
    fi
	return 0
}

function updateG2H2() {
	if [ -f  ${SYS_FILE} ]; then		
        	/usr/bin/psplash-write "HDR_MSG save original ..."
        	cp /etc/init.d/version.sh /application/.
		sync
		sleep 2s
		cp $SYS_FILE /etc/init.d/
		sync
		sleep 1s
		if [ -e "/etc/init.d/${SYS_FILE_NAME}" ]; then
			/usr/bin/psplash-write "PROGRESS 100"
			/usr/bin/psplash-write "MSG sys updated. Next step continue ..."
			sleep 2s
		fi	
	fi
}

function checkCurrentUIrev() {
	echo "check current rev. info"
	currentRev=$(cat < /application/current_uirev)
	currentRev=${currentRev%$RT}
	echo " the current revision is: $currentRev"
	
}

function updateReachDisplay() {
    status=-1

	if [ -f  ${REACH_DISPLAY_PATH} ]; then
    
        if [ -f  ${REVFILE} ]; then
            qmlRev=$(cat < '/run/media/'${REACH_DISPLAY_FILE}/'UI_rev')
            # $RT is alias of '\r' and is used to remove any trailing space
            qmlRev=${qmlRev%$RT}
			checkCurrentUIrev
            /usr/bin/psplash-write "HDR_MSG current: $currentRev to new: $qmlRev"
			echo -n "$qmlRev" > /application/current_uirev
            
        fi    
        /usr/bin/psplash-write "MSG Updating Display Module Software (about 1 minute)..."
		sleep 2s

        /usr/bin/psplash-write "MSG save settings ..."
		cd ${APPLICATION_PATH}
        cp /application/src/application.conf /application/.
		sync
		/usr/bin/psplash-write "PROGRESS 5"

		/usr/bin/psplash-write "MSG preparing 1 ..."
		cp -vf ${REACH_DISPLAY_PATH} ${APPLICATION_PATH}
        sleep 1s
		sync
		/usr/bin/psplash-write "PROGRESS 10"

 	if [ -f "$REACH_DISPLAY_FILE_NAME" ]; then
		if [ -d ./src ]; then
		/usr/bin/psplash-write "MSG preparing 2 ..."
            rm -rf './src_backup'
            sync
            /usr/bin/psplash-write "PROGRESS 20"
        	mv './src' './src_backup'
            sync
		fi
        
		sleep 1s	
		/usr/bin/psplash-write "MSG preparing 3 ..."
		g2h2Progress &
        #/usr/bin/psplash-write "PROGRESS 30"
		tar -xzvf /application/sciton_reach_update.tgz
        status=$?

		if [[ status -eq 0 ]]; then
			/usr/bin/psplash-write "MSG g2h2 file expansion done."
			/usr/bin/psplash-write "PROGRESS 40" 
			#G2H2_FILESTAT="$(zcat /application/sciton_reach_update.tgz | wc -c)"

			if [ -d ./src ]; then 
				/usr/bin/psplash-write "MSG preparing 4 ..."			
				rm ${APPLICATION_PATH}${REACH_DISPLAY_FILE_NAME}
                /usr/bin/psplash-write "MSG preparing 5 ..."
                sync
 				/usr/bin/psplash-write "PROGRESS 60"               
				sleep 1s
                /usr/bin/psplash-write "MSG preparing 6 ..."                
                /usr/bin/psplash-write "PROGRESS 80"
				/usr/bin/psplash-write "MSG Restoring the application settings ..."
        		cp /application/application.conf /application/src/.
				/usr/bin/psplash-write "PROGRESS 90"

				sync
				/usr/bin/psplash-write "MSG Finalizing Display Module components ..."					
				#/usr/bin/psplash-write "MSG ${G2H2_FILESTAT}"
        		/usr/bin/psplash-write "PROGRESS 100"
                display_updated=1
			else
				/usr/bin/psplash-write "MSG Display Module update (2) failed. Please try again."
                display_updated=-2
            fi
		else
			/usr/bin/psplash-write "MSG Display Module update (1) failed. Please try again."
            display_updated=-1
        fi
        
		sleep 2s
       	/usr/bin/psplash-write "MSG Start next step. Please wait..."
       	sleep 2s	
		
	fi
	else
		/usr/bin/psplash-write "MSG Display Module File not available."
        sleep 1s
	fi
}

echo off
echo "Check for firmware files ..."
if [ "${REACH_DISPLAY_FILE//[[:blank:]]/}" ]; then
    updateG2H2
    updateReachDisplay
    files_exist=1  
    if [[ $display_updated -eq 1 ]]; then
        /usr/bin/psplash-write "MSG Display Module update completed"
    fi
    sleep 1s    
fi

if [ "${SCITON_FW_IMG_EXT//[[:blank:]]/}" ]; then   
    currentRev=""
    REVFILE='/run/media/'${REACH_DISPLAY_FILE}/'controller_rev'
        if [ -f  ${REVFILE} ]; then
            currentRev=$(cat < '/run/media/'${REACH_DISPLAY_FILE}/'controller_rev')
            # $RT is alias of '\r' and is used to remove any trailing space
            currentRev=${currentRev%$RT}
			#checkCurrentCtrlrev
            /usr/bin/psplash-write "HDR_MSG update to new: $currentRev"
			echo -n "$currentRev" > /application/current_contrlrev            
        fi 
    updatePhytecModule
    if [[ $phytec_updated -eq 1 ]]; then
        /usr/bin/psplash-write "MSG joule System software update completed"
    fi
    sleep 3s     
    files_exist=2
fi

if (( files_exist < 1 )); then
	/usr/bin/psplash-write "MSG System will continue in 4 seconds. (system-update file not exist, or USB Drive NOT inserted or recognized.)"
    sleep 4s
    /usr/bin/psplash-write "QUIT"
fi
  
/usr/bin/psplash-write "MSG Please remove the USB drive now ... The System shall restart in 5 seconds ..."
sleep 5s
/usr/bin/psplash-write "QUIT"
