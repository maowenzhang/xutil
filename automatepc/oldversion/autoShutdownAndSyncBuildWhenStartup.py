# Used to create tasks to task scheduler
# 1. create task: auto shutdown computer
# 2. create task: auto sync and build codes
#

#==================================
# Configuration
# 
import os
import getpass

class Tasks:
    def __init__(self, userInfo):
        self.shutdownPCTask = TaskInfo("autoShutdownPC", "shutdown.bat", "daily", "18:00")
        self.autoSyncBuildTask = TaskInfo("autoSyncBuild", "autoSyncBuild_main.py", "onstart", '')
        if userInfo:
            self.autoSyncBuildTask.setUserInfo(userInfo)

class TaskInfo:
    def __init__(self, name, taskrun, schedule, starttime):
        self.name = name
        self.schedule = schedule
        self.starttime = starttime
        self.taskrun = taskrun
        self.taskrun = os.path.join(os.path.dirname(os.path.realpath(__file__)), taskrun)
        print('task run: \t', self.taskrun)
        self.username = ""
        self.userpassword = ""
    
    def setUserInfo(self, userInfo):
        self.username = userInfo.name
        self.userpassword = userInfo.password

    def getTaskCreateCmd(self):
        tmp = "schtasks /create /f /tn {0} /tr {1} /sc {2}"
        cmd = tmp.format(self.name, self.taskrun, self.schedule)

        # add user info
        if self.username and self.userpassword:
            cmd = "{0} /ru {1} /rp {2}".format(cmd, self.username, self.userpassword)
        if self.starttime:
            cmd = "{0} /st {1}".format(cmd, self.starttime)

        print(cmd)
        return cmd

    def getTaskDeleteCmd(self):
        tmp = "schtasks /delete /f /tn {0}"
        cmd = tmp.format(self.name)
        print(cmd)
        return cmd

class UserInfo:
    def __init__(self):
        self.name = getpass.getuser()
        print('user name: \t', self.name)
        print('password is required to create task in windows task scheduler')
        self.password = getpass.win_getpass()

#==================================
# Run task
# 
import subprocess

def runcmd(cmd):
    p = subprocess.Popen(cmd)
    p.wait()

def createTasks(tasks):
    print("\n============== Create tasks to task scheduer:\t")
    print("\n-------------- create task to auto shutdown PC")
    runcmd(tasks.shutdownPCTask.getTaskCreateCmd())
    print("\n-------------- create task to auto sync build codes")
    runcmd(tasks.autoSyncBuildTask.getTaskCreateCmd())   

def deleteTasks(tasks):
    print("\n============== Delete tasks from task scheduer:\t")
    print("\n-------------- delete task to auto shutdown PC")
    runcmd(tasks.shutdownPCTask.getTaskDeleteCmd())
    print("\n-------------- delete task to auto sync build codes")
    runcmd(tasks.autoSyncBuildTask.getTaskDeleteCmd())

def run(isCreateTask):

    if isCreateTask:
        userInfo = UserInfo()
        tasks = Tasks(userInfo)
        createTasks(tasks)
    else:
        tasks = Tasks('')
        deleteTasks(tasks)

#==================================
# Run
#
import argparse
if __name__ == '__main__':
    print('\n\n************************************************************************')
    print('*************** START autoShutdownAndSyncBuildWhenStartup \n')

    parser = argparse.ArgumentParser()
    parser.add_argument('isCreateTask', type=int, default=[1], nargs='*')
    args = parser.parse_args()
    isCreateTask = args.isCreateTask[0];
    if isCreateTask:
        print('input ', isCreateTask, ' create/enable tasks')
    else:
        print('input ', isCreateTask, ' delete/disable tasks')

    run(isCreateTask)

    input("\n(run as admin if access denied) press enter to exit ....")

#run()

