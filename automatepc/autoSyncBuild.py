#==================================
# autoSyncBuild
#
# Used to sync latest codes and build them
# based on information (name, commands) in config.json file
#
# Usage: 
#       1. add synbuild info (name, commands) in config.json file
#       2. run script: "autoSyncBuild.py"
#       3. add this script to windows task scheduler via main.bat
#
# Comments:
#       1. May adjust task commands by adding user info, file path, etc.
#
# By zhanglo, 5/20/2013

import taskrunner;
import os, json

def loadConfig():
    file = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'config.json')
    f = open(file)
    config = json.load(f)
    #print(config)
    return config;

def run():
    config = loadConfig()
    syncbuildconfig = config.get("syncbuild")
    if not syncbuildconfig:
        print("find no item: syncbuild in config.json")
        return
    taskrunner.run(syncbuildconfig)

run()
input('<press enter to exit>......')
