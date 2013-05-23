#===============================
# autoSyncBuild V1.0
#
# This script is used to auto sync codes and call script to rebuild
# : sync to latest codes -> rebuild codes
#
# Usages:
#       1. set code path, log file, tasks as below
#       2. add this to windows task schedule to make it run when PC startup
#       
# By zhanglo, 5/20/2013

#===============================
# Configuration
#
# 1. NOT use file path with virtiral mapped disk such as "W:\", or taskSchedule will fail to find it
# 2. NOT include commands start UI such as StartWin64DebugVisualStudio_CMake.py. 
#    Because adding this script to windows task schedule and running it when not log on, the script 
#    is run in background, so UI will be invisible.
#
# code path used to sync latest codes in perforce
codepath = r'//depot/Neutron/main/...'

# build path used to call build script
buildpath = r'E:\Neutron\main\src\fusion\Build\Windows'

import os
config = {
          # log file
            "logpath"   :   os.path.join(os.path.dirname(os.path.realpath(__file__)), "autoSyncBuild.log")
          
          # tasks to run in order
          , "runtasks"  : [
                           # sync to latest codes
                              r'p4.exe sync ' + codepath + r'#head'
                           # rebuild codes
                            , "python.exe " + buildpath + r"\BuildWin64Debug_CMake.py"
                          ]
          }

#===============================
# RUN
#
import taskrunner
#print(config)
taskrunner.run(config)

#input('<press enter to exit>......')