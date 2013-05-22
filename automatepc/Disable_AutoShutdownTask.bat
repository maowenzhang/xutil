echo off
echo _
echo ============= Disable auto shutdown pc task ============
echo _
echo _
echo ------------- #. delete task "autoShutdownPC"
schtasks /delete /f /tn autoShutdownPC
echo _
echo _
echo if access denied, please right click bat to run as admin
pause