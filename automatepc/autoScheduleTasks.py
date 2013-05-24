from datetime import datetime
import subprocess, sys

#===============================
# Get commands
#
cmd = r'c:\Windows\System32\schtasks.exe /create /tn testt4 /ru ads\zhanglo /rp #1223z0612y* /sc onstart /tr notepad.exe'


#===============================
# Run tasks
#
def runcmdtest(cmd):
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    p.wait()
    print(p.stdout.read())
    print(p.stderr.read())
        
def runcmd(cmd):
    try:
        p = subprocess.Popen(cmd, stdout=sys.stdout, stderr=sys.stdout)
        p.wait()
    except:        
        print('Fail to run cmd: ' + cmd)
        return False
    return True

def runcmds(cmds):
    print('\nCommands:')
    if len(cmds) == 0:
        print("No commands to run")
        return
    
    for c in cmds:
        print('\t\t' + c)

    for c in cmds:
        print("\n============== Run cmd:\t" + c)        
        ret = runcmd(c)
        if not ret:
            break

def run():
    print('\n\n************************************************************************')
    print('*************** START autoScheduleTasks: ' + str(datetime.now()))

    #runcmds(RUNTASKS)
    runcmdtest(cmd)

    print('\n\n*************** END autoScheduleTasks: ' + str(datetime.now()))

#===============================
# Run
#
run()

#input('<press enter to exit>......')
