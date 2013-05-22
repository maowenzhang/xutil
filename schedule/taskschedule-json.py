import time
import sched
import json
import datetime
import sched
import os

g_weekdays = ["Monday", "Tuesday", "Wednsday", "Thursday", "Friday", "Saturday", "Sunday"]	

def func1():
    """ doc string, used to test doc string"""
    print('name: func1')
    print('doc string: ', func1.__doc__)

class ConfigInfo:
    def __init__(self, runtime, runtask, rundays):
        self.runtime = {"hour":int(runtime[0]), "minute":int(runtime[1])}
        self.runtask = runtask
        self.rundays = rundays
    def print(self):
        print(self.runtime, self.runtask, self.rundays)

def readconfig():
    """read task schedule config in file "taskscheduleconfig.txt"""
    
    f = open('taskscheduleconfig.txt')
    c = json.load(f)
    print(c)

    runtime = c.get('runtime');
    if not runtime:
        print("no specified runtime in config file: taskscheduleconfig.txt")
        return (False, None)
    runtime = runtime.split(':')
    
    runtask = c.get('runtask')
    if not runtask:
        print("no specified runtask in config file: taskscheduleconfig.txt")
        return (False, None)
    rundays = c.get('rundays')
    if not rundays:
        print("no specified rundays in config file: taskscheduleconfig.txt")
        return (False, None)
    
    ci = ConfigInfo(runtime, runtask, rundays)
    return (True, ci)

def getNextRuntime(ci):
    """get next time to run task"""
    # get today time
    today = datetime.datetime.today()
    print(today)
    weekdayIndex = today.weekday()
    weekdayStr = g_weekdays[weekdayIndex]
    print(weekdayStr)

    # get runday delta (runday - today)
    rundayDelta = 0
    rundaysSelected = [k for k, v in ci.rundays.items() if v]
    print("rundays selected: ", ";".join([x for x in rundaysSelected]))
    while not ci.rundays.get(weekdayStr):
        rundayDelta += 1
        weekdayIndex += 1
        weekdayIndex %= 7
        weekdayStr = g_weekdays[weekdayIndex]
    print("run day: ", weekdayStr)
    print("day delta: ", rundayDelta)

    # get run date time   
    # testing
    rundayDelta = 0
    ci.runtime['hour'] = 13
    ci.runtime['minute'] = 46  
    
    nextRuntime = datetime.datetime(today.year, today.month, \
                                    today.day + rundayDelta, \
                                    ci.runtime['hour'], ci.runtime['minute'])
    return nextRuntime

def scheduleAction():
    print("============== run task......")
    #os.execlp('notepad.exe', '')
    os.execlp(r'notepad.exe', r'c:\a.txt')

def scheduletask(ci, nextRuntime):
    s = sched.scheduler(time.time, time.sleep)
    print("shedule time: ", nextRuntime)
    s.enterabs(nextRuntime.timestamp(), 1, scheduleAction)
    print("============== task scheduled")
    s.run()
    print("============== finish running")
    
def run():
    # 1. read config
    res = readconfig()
    if not res[0]:
        return
    ci = res[1]
    ci.print()

    # 3. get next run time
    nextRuntime = getNextRuntime(ci)
    print(nextRuntime)
    if nextRuntime < datetime.datetime.now():
        print("schedued time is passed!")
        #return

    # 2. schedule task
    scheduletask(ci, nextRuntime)    
    

run()

    
    
    
    
