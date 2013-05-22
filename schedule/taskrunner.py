import subprocess
import logging
import sys
from datetime import datetime
logging.basicConfig(filename='taskrunner.log', level=logging.DEBUG)

headinfo = "\n\n================= Start task runner  " + str(datetime.now()) + "\n"
logging.info(headinfo)
print(headinfo)

def getinputcmds():    
    if len(sys.argv) == 1:
        return []
    cmds = sys.argv[1:]
    return cmds

def runcmd(cmd):
    try:
        subprocess.call(cmd)
    except:        
        logging.error('fail to run cmd: ' + cmd)
        print('fail to run cmd: ' + cmd)
        return False
    return True

def run(cmds):
    logging.info('commands:')
    if len(cmds) == 0:
        logging.info("no commands to run")
        return
    
    for c in cmds:
        logging.info('\t\t' + c)
    
    starttime = datetime.now()

    for c in cmds:
        logging.info("========= run cmd:\t" + c)
        print("========= run cmd:\t" + c)
        logging.info("run started:\t" + str(datetime.now()))
        if not runcmd(c):
            break
        logging.info("run ended:\t" + str(datetime.now()))
        
    endtime = datetime.now()
    runtime = endtime - starttime
    logging.info("run time: \t" + str(runtime))

    input("pause......")

#=============================
# run
cmds = getinputcmds()
run(cmds)



