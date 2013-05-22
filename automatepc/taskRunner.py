#===============================
# taskRunner V1.0
#
# This script is used to auto run tasks
#
# Usages:
#       input config with commands to run
#       refer to example 'autoSyncBuild_main.py'
# By zhanglo, 5/20/2013

import sys
from datetime import datetime
import subprocess
import shutil

#===============================
# configuration
#
class ConfigInfo:
    def __init__(self, config):
        self.logpath = config.get("logpath")
        self.runtasks = config.get("runtasks")

#===============================
# Output info to log file
#
class RedirectOutputToFile:
    def __init__(self):
        pass
    
    def setLogfile(self, logfile):
        self.out_file = open(logfile, 'w')  
        
        self.logfile = logfile      
        curtimestr = '__{:%Y-%m-%d_%H-%M}.log'.format(datetime.now())
        self.logfileWithTime = logfile + curtimestr
            
    def __enter__(self):
        self.out_old = sys.stdout
        self.err_old = sys.stderr

        sys.stdout = self.out_file
        sys.stderr = self.out_file

    def __exit__(self, *args):
        self.out_file.close()
        sys.stdout = self.out_old
        sys.stderr = self.err_old

        shutil.copyfile(self.logfile, self.logfileWithTime)

        print('exit...')
        
    def flush(self):
        self.out_file.flush();

REDIRECTOUT = RedirectOutputToFile()

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

def runex(config):
    configInfo = ConfigInfo(config)

    starttime = datetime.now()
    print('\n\n************************************************************************')
    print('*************** START taskRunner: ' + str(starttime))

    runcmds(configInfo.runtasks)

    endtime = datetime.now()
    runtime = endtime - starttime
    print('\n\n*************** END taskRunner: ' + str(endtime))
    print('\n*************** TIME taskRunner: ' + str(runtime))

#===============================
# Run
#
def run(config):
    REDIRECTOUT.setLogfile(config.get("logpath"))
    with REDIRECTOUT:
        runex(config)

#input('<press enter to exit>......')
