automatepc
5/23/2013, zhanglo
===============================

Summary:
	1. Some scripts to auto shutdown pc, run tasks (sync and build codes) when pc startup, etc.
	2. It gets settings in config.json, and create/delete tasks in windows task scheduler.

Usages:
	1. update config.json
		set code path in perforce, commands to run, etc.
	2. run "main.bat", which enables tasks (autoShutdown&syncBuildCodes in config.json)
		If access denied, try right click "run as admin"
	3. run the bat files when needed to enable/disable scheduled tasks
		Enable_autoShutdown.bat
		Disable_autoShutdown.bat
		Enable_autoShutdownAndSyncBuild.bat
		Disable_autoShutdownAndSyncBuild.bat


	