#===============================
# autoSyncBuild V1.0
#
# This script is used to auto sync codes and call script to rebuild
# sync to latest codes -> rebuild codes
#
# Usages:
#       1. set location of codes and log file as below
#       2. create a task in task schedule to run this script when computer startup (without log-on)
#       3. modify BIOS (F10->BIOS->menu (Advanced->BIOS Power-On)->set when to start pc)
#       4. copy shortcut "StartWin64DebugVisualStudio_CMake.py" in pc startup folder to start it when log-on
#       
# By zhanglo, 5/20/2013

#===============================
# configuration
#
# Sync latest codes
CODEPATH = r'//depot/Neutron/main/...'

# Location of build scripts (NOT using virtiral mapped disk, or taskSchedule will fail!)
BUILDPATH = r'E:\Neutron\main\src\Fusion\Build\Windows'

# Add datetime at the end of file name (NOT using virtiral mapped disk, or taskSchedule will fail!)
LOGPATH = r'C:\Users\zhanglo\Desktop\Tool\autoSyncBuild_V1_main'

# Run tasks
RUNTASKS = [
    r'p4.exe sync ' + CODEPATH + r'#head'
    , "python.exe " + BUILDPATH + r"\BuildWin64Debug_CMake.py"
    # As task schdule is run in background process, useless to start VS
    #, "python.exe " + BUILDPATH + r"\StartWin64DebugVisualStudio_CMake.py" 
    ]


import sys
from datetime import datetime
import subprocess

#===============================
# Output info to log file
#
class RedirectOutputToFile:
    def __init__(self, logfile):
        curtimestr = '__{:%Y-%m-%d_%H-%M}.log'.format(datetime.now())
        logfile += curtimestr
        
        self.out_file = open(logfile, 'w')        
        
    def __enter__(self):
        self.out_old = sys.stdout
        self.err_old = sys.stderr

        sys.stdout = self.out_file
        sys.stderr = self.out_file

    def __exit__(self, *args):
        self.out_file.close()
        sys.stdout = self.out_old
        sys.stderr = self.err_old
        print('exit...')
        
    def flush(self):
        self.out_file.flush();

REDIRECTOUT = RedirectOutputToFile(LOGPATH)

def flushOutput():
    if REDIRECTOUT:
        REDIRECTOUT.flush()
        
  
def printwarning(str):
	print("!!!!!!!!!!!!!!!!!!!!!\t", str)

#===============================
# Run tasks
#	
def runcmd(cmd):
    try:
        p = subprocess.Popen(cmd, stdout=sys.stdout, stderr=sys.stdout)
        p.wait()        
    except:        
        printwarning('Fail to run cmd: ' + cmd)
        return False
    return True

def runcmds(cmds):
    flushOutput()
    print('\nCommands:')
    if len(cmds) == 0:
        printwarning("No commands to run")
        return
    
    for c in cmds:
        print('\t\t' + c)

    for c in cmds:
        print("\n============== Run cmd:\t" + c)
        starttime = datetime.now()
        print("run start:\t" + str(starttime))
        flushOutput()
        
        ret = runcmd(c)
        
        flushOutput()
        endtime = datetime.now()
        print("run end:\t" + str(datetime.now()))
        runtime = endtime - starttime
        print("run time: \t" + str(runtime))

        if not ret:
            break

def run():
    starttime = datetime.now()
    print('\n\n************************************************************************')
    print('*************** START autoSyncBuild: ' + str(starttime))

    runcmds(RUNTASKS)

    endtime = datetime.now()
    runtime = endtime - starttime
    print('\n\n*************** END autoSyncBuild: ' + str(endtime))
    print('\n*************** TIME autoSyncBuild: ' + str(runtime))

#===============================
# Run
#

with REDIRECTOUT:
    run()

#input('<press enter to exit>......')
