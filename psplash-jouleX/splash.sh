#! /bin/bash
echo " this script builds the executable 'psplash' "
make V=1
echo " if no error so far then rename'pspash' to 'psplash-default' "
echo " then copy it to the target at /usr/bin/ "
echo " don't foget to change 'psplash-default' permission to user executable!" 





