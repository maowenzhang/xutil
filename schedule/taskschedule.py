import time
import sched
import json
import datetime
import sched
import os
import subprocess

#=======================================================
# Cofigration
#
class ConfigInfo:
    def __init__(self, runtime, runtasks, rundays):
        self.runtime = {"hour":int(runtime[0]), "minute":int(runtime[1])}        
        self.rundays = rundays
        self.runtasks = runtasks
        self.rundaysSelected = [k for k, v in rundays.items() if v]
    def print(self):
        print('runtime:', str(self.runtime['hour']) + ":" + str(self.runtime['minute']), sep='\t')        
        print('rundays:', self.rundaysSelected, sep='\t')
        print('runtasks:')
        print("\t\t" + "\n\t\t".join(self.runtasks))

def readconfig(config):
    """read task schedule config in file "taskscheduleconfig.txt"""
    
    #f = open('taskscheduleconfig.txt')
    #c = json.load(f)
    #print(c)

    c = config

    runtime = c.get('runtime');
    if not runtime:
        print("ERROR: no specified runtime in config")
        return (False, None)
    runtime = runtime.split(':')
    
    runtasks = c.get('runtasks')
    if not runtasks:
        print("ERROR: no specified runtasks in config")
        return (False, None)
    rundays = c.get('rundays')
    if not rundays:
        print("ERROR: no specified rundays in config")
        return (False, None)
        
    ci = ConfigInfo(runtime, runtasks, rundays)
    ci.print()
    return (True, ci)

#=======================================================
# Get sheduled task run time
#
g_weekdays = ["Monday", "Tuesday", "Wednsday", "Thursday", "Friday", "Saturday", "Sunday"]

def getNextRuntime(ci):
    """get next time to run task"""
    # get current day used to check
    curday = datetime.datetime.today()    
    print('Current time: \t', curday)
    
    # move to 2nd day if the time is passed
    timepassed = False
    if curday.hour > ci.runtime['hour']:
        timepassed = True
    elif curday.hour == ci.runtime['hour']:
        if curday.minute >= ci.runtime['minute']:
            timepassed = True
    if timepassed:
        print('Today is passed')
        curday = curday + datetime.timedelta(days=1)
        
    weekdayIndex = curday.weekday()
    weekdayStr = g_weekdays[weekdayIndex]
    #print(weekdayStr)

    # get runday delta (runday - curday)
    rundayDelta = 0
    rundaysSelected = [k for k, v in ci.rundays.items() if v]
    print("days selected: \t", ";".join([x for x in rundaysSelected]))
    while not ci.rundays.get(weekdayStr):
        rundayDelta += 1
        weekdayIndex += 1
        weekdayIndex %= 7
        weekdayStr = g_weekdays[weekdayIndex]
    print("run day: \t", weekdayStr)
    #print("run day delta: \t", rundayDelta)

    # get run date time   
    # testing
    #rundayDelta = 0
    #ci.runtime['hour'] = 15
    #ci.runtime['minute'] = 49
    #print(curday.day, rundayDelta)
    
    nextRuntime = curday + datetime.timedelta(days=rundayDelta)    
    nextRuntime = datetime.datetime(nextRuntime.year, nextRuntime.month, \
                                    nextRuntime.day, \
                                    ci.runtime['hour'], ci.runtime['minute'])
    return nextRuntime

#=======================================================
# Shedule task
#
def scheduleAction(ci):
    print("Start task at: \t", datetime.datetime.now())

    # direclty run command
    if len(ci.runtasks) == 1:
        tmp = ci.runtasks[0]
        print(r'run task:\t', tmp)
        subprocess.Popen(tmp)
        return

    # use extra python file to run commands
    tmp = 'python taskrunner.py'
    for i in ci.runtasks:
        tmp += ' \"' + i + '\"'
    print('run task:\t', tmp)
    #os.system(tmp)
    subprocess.Popen(tmp, shell=False, creationflags=subprocess.CREATE_NEW_CONSOLE)
    #os.startfile(tmp)

def scheduletaskItem(ci, nextRuntime):
    now = datetime.datetime.now()
    
    s = sched.scheduler(time.time, time.sleep)
    print("Schedule time: \t", nextRuntime)
    
    t1 = time.mktime(nextRuntime.timetuple())
    t2 = time.mktime(now.timetuple())
    delaytime = t1 - t2
    print('waiting time: \t', nextRuntime - now)
    
    s.enter(delaytime, 1, scheduleAction, [ci])
    print('Waiting......')
    s.run()
    print("Finish this task")

def scheduletask(ci):
    print('\n========= #. Get next run time')
    nextRuntime = getNextRuntime(ci)
    #print('Run time: \t', nextRuntime)
    if nextRuntime < datetime.datetime.now():
        print("ERROR: schedued time is passed!")
        return
    
    scheduletaskItem(ci, nextRuntime)

    print('\n************** # 2.. schedule task AGAIN')
    scheduletask(ci)  

   
#=======================================================
# Run, main entry
#   
def run(config):
    """main entry to run script"""
    
    print('\n************** # 1. Read config')
    res = readconfig(config)
    if not res[0]:
        return
    ci = res[1]

    print('\n************** # 1.1. Run immediately?')
    res = input('Need to run immediately? (enter y to run): ')
    res = res.strip()
    print(r'user entered: "' + res + r'"')
    if res == 'y':
        scheduleAction(ci)

    print('\n************** # 2. schedule task')
    scheduletask(ci)

#=======================================================
# test
#
def testp4():
    print('run test')
    input('<enter to exit>...')

    
#if __name__ == '__main__':
    
    

    
    
    
    
