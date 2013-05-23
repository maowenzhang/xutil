#================================
# main entry to config
#

import json
import subprocess
import os

def loadConfig():
    file = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'config.json')
    f = open(file)
    config = json.load(f)
    #print(config)
    return config;

def createAutoShutdownSyncBuildTasks():
    print("\n--------------- create AutoShutdownSyncBuildTasks\n")
    file = "Enable_autoShutdownAndSyncBuild.bat"
    if not os.path.exists(file):
        print("find no file to run: ", file)
    print("run file: ", file)
    subprocess.call(file)

def createShortcutBatchFile(tasks):
    print("\n--------------- create shotcub bat files\n")
    testfile = open('scheduletaskhelper__test.bat', 'w')
    for t in tasks:
        name = t.get("name")
        if not name:
            print('find no name set in task: ', t)
        
        print("create file: " + name, '.bat')
        f = open(name + '.bat', 'w')
        schedulefile = os.path.join(os.path.dirname(__file__), "scheduletaskhelper.py")
        #schedulefile = "scheduletaskhelper.py"
        output = 'python.exe {0} {1}\n'.format(schedulefile, name)
        f.write(output)
        f.close()
        testfile.write(output)
    testfile.close()

def run():
    print('\n\n************************************************************************')
    print("================== Main to config tasks\n")
    config = loadConfig()
    tasks = config.get("scheduletasks")
    if not tasks:
        print("find no item: scheduletasks in config.json")
        return

    createShortcutBatchFile(tasks)

    createAutoShutdownSyncBuildTasks()

run()
input("press enter to exit...")