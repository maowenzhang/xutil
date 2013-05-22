echo off
echo _
echo =============  create auto shutdown pc task ============
echo PS: rember to update path of the shutdown.bat with contents (c:\Windows\System32\shutdown.exe /s /t 0 /f)
echo _
echo _
echo ------------- #. create the task (override if has)
schtasks /create /f /tn autoShutdownPC /sc daily /st 18:00 /tr C:\Users\zhanglo\Desktop\shutdown.bat
echo _
echo _
echo if access denied, please right click bat to run as admin
pause