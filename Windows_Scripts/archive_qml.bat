@echo on
@echo.
@echo This utility prepares a file 'sciton_reach_update.tgz' for Display software update via usb
@echo -------------------------------------------------------------------------------------------

@echo off
rem Use Windows 7Zip utility to archive g2h2 qml file set for usb download
rem double click to run this file 'archive_qml.bat' within ...\qml directory
rem Syntax: archive_qml
rem The operation should render a file 'sciton_reach_update.tgz' in this same directory
rem The file size is about 1 Mb 
rem Default compression method is 'Deflate' for 'zip' compression format. 
rem Copy this resulted compressed file to your usb FW installer drive.

rem backup and rename the existing tar file at first otherwise 7z will fail.

cmd.exe /c

echo File detection phase:

echo Attempt to obtaining revision information ...
call buildrevstr.bat
if exist "UI_rev"     (
    echo Attach this 'UI_rev' file to the USB drive after this utility completes.
) else     (
    echo Failed to find revision information.
)

echo ----------------------------------
echo create compressed file:
if exist "src.tar" (
@echo on
   echo delete temporary file 'src.tar'
   del src.tar
)
if exist ".\sciton_reach_update.tgz" (
@echo on
   @echo Step 1. move the tar file to a backup copy -----------
   move ".\sciton_reach_update.tgz" "..\sciton_reach_update.tgz_backup"
)
@echo off
PING localhost -n 2 >NUL
@echo on
@echo Step 2. compressing G2H2 files ... ----------------------
@echo off

rem gzip does not compress multiple files, so archive multiple files first with -ttar
rem The following command line archives all files in \src to the parent directory
rem 7z -ttar a src .\* -so | 7z -si -tgzip a ..\sciton_reach_update.tgz
rem -so redirects the output to stdout and pipe the output to the -si (input) option
rem This creates a sciton_reach_update.tgz containing all data from current directory

C:\Utilities\7-Zip\7z -x@..\excludedfilelist.txt -ttar a .\src  
C:\Utilities\7-Zip\7z -tgzip a sciton_reach_update.tgz src.tar

if exist "src.tar" (
   del src.tar
)

@echo Step 3. Coying file wait for 5 seconds ... ----------
rem move "..\sciton_reach_update.tgz" .\
ping -n 5 127.0.0.1 > NUL


@echo Done ------------------------------------------------
pause