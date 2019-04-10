#! /bin/bash
echo " Navigate to the project folder"
cd ~/Projects/psplash-jouleX
echo " Get a reference to the compiler"
source /opt/reach/1.6/environment-setup-cortexa9hf-vfp-neon-reach-linux-gnueabi
echo " Creat a build environment"
./autogen.sh --host=arm-reach-linux-gnueabi
echo " if no error so far then execute the build script 'spash.sh' "




