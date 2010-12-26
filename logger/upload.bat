@echo off
setlocal enabledelayedexpansion
set d=%SYSTEMDRIVE:~0,1%
set p=%SYSTEMROOT:~3,7%

"%ProgramFiles%\logger\rsync\rsync.exe" -azv --port 1873 /cygdrive/%d%/%p%/system32/LogFiles/tdifw/ rome.rutgers.edu::log/%COMPUTERNAME%