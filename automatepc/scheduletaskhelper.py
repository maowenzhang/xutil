#==================================
# sheduletaskhelper
#
# Used to create/delete tasks to/from windows task scheduler
# based on task information (name, commands) in config.json file
#
# Usage: 
#       1. add task info (name, commands) in config.json file
#       2. run script: "sheduletaskhelper.py taskname"
#
# Comments:
#       1. May adjust task commands by adding user info, file path, etc.
#
# By zhanglo, 5/20/2013

#==================================
# User info
# 
import os
import getpass
import json
import argparse
import subprocess
import re

class UserInfo:
    def __init__(self):
        self.name = getpass.getuser()
        print('user name: \t', self.name)
        print('password is required to create task in windows task scheduler')
        self.password = getpass.win_getpass()

#==================================
# Run task
# 
def runcmd(cmd):
    print("\n------------ Run command")
    p = subprocess.Popen(cmd)
    p.wait()

def printcmds(cmds):
    print("Commands:")
    for cmd in cmds:
        print("\t", cmd)

def adjustcmd(cmd):
    print("\n------------ Adjust command")
    newcmd = cmd
    # add taskrun file full path
    m = re.search(r'[\s\t][a-zA-Z0-9_-]+\.[a-zA-Z]{1,4}', cmd)
    if m:
        file = m.group(0).strip()
        print("found matched file: ", file)
        newfilepath = os.path.join(os.path.dirname(os.path.realpath(__file__)), file)
        newcmd = newcmd.replace(file, newfilepath)
        print("added file full path")
    
    # add user and password if need
    tag = 'RunWithoutLogon'
    res = newcmd.find(tag)
    if res != -1:
        userinfo = UserInfo()
        newcmd = newcmd.replace(tag, '')
        if userinfo.name and userinfo.password:
            newcmd = "{0} /ru {1} /rp {2}".format(newcmd, userinfo.name, userinfo.password)
            print("added user name and password")

    return newcmd

def runtask(task):
    # get settings
    name = task.get("name")
    if not name:
        print("find no name of task")
        return
    summary = task.get("summary")
    commands = task.get("commands")
    if not commands:
        print("find no commands of task")
        return
    print("\n==================== Run Task: ", name, "\n")
    print("Summary:\t{0}".format(summary))
    printcmds(commands)
    
    # adjust commands
    adjustedcmds = []
    for cmd in commands:
        adjustedcmds.append(adjustcmd(cmd))
    printcmds(adjustedcmds)

    # run
    for cmd in adjustedcmds:
        runcmd(cmd)

def loadConfig():
    file = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'config.json')
    f = open(file)
    config = json.load(f)
    #print(config)
    return config;

def gettask(taskname):
    config = loadConfig()
    tasks = config.get("scheduletasks")
    if not tasks:
        print("find no item: scheduletasks in config")
        return
    for t in tasks:
        name = t.get("name")
        if name:
            if name == taskname:
                return t
    return ""

def runex(taskname):
    task = gettask(taskname)
    if not task:
        print("find no the task in config.json: ", taskname)
        return
    
    runtask(task)

#==================================
# Run
#
def run():
    print('\n\n************************************************************************')
    print('*************** START scheduleTaskHelper \n')
    
    parser = argparse.ArgumentParser()
    parser.add_argument('taskname', help='task name set in config.json')
    args = parser.parse_args()

    runex(args.taskname)

    input("\n(run as admin if access denied) press enter to exit ....")

run()

def test():
    #runex("Enable_autoShutdown")
    #runex("Disable_autoShutdown")
    runex("Enable_autoShutdownAndSyncBuild")
    #runex("Disable_autoShutdownAndSyncBuild")
    input("pause")

#test()