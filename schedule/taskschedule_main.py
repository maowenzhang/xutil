#===============================
# Readme
#
# This script is used to update builds to latest at selected time/days
# sync to latest codes -> build codes -> open visual studio
# Usages: just need to adjust the configuration and run this file
# By zhanglo, 5/2/2013

#===============================
# configuration
#
# code path used to sync latest codes in perforce
codepath = r'//depot/Neutron/main/...'
# build path used to call build script and open visual studio script
buildpath = r'E:\Neutron\main\src\fusion\Build\Windows'

# config data passed to taskschedule module
config = {
        # the time to run task, such as 6:30 at morning
	"runtime" : "6:30"
        # the selected days to run task (1 means to run) 
	, "rundays" : {
			  "Monday" : 1                          
			, "Tuesday" : 0
			, "Wednsday" : 1
			, "Thursday" : 0
			, "Friday" : 1
			, "Saturday" : 0
			, "Sunday" : 0
                        }
        # the tasks to run (could be commands, files, etc.)
        , "runtasks" : [
                          "p4.exe sync " + codepath + "#head"
                        , "python.exe " + buildpath + r"\BuildWin64Debug_CMake.py"
                        , "python.exe " + buildpath + r"\StartWin64DebugVisualStudio_CMake.py"
                        ]
}

#===============================
# Run
#
import taskschedule
taskschedule.run(config)

input('<press enter to exit>......')
