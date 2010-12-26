@echo off

md %SystemRoot%\system32\LogFiles\tdifw > nul

echo default config
copy /y tdifw.conf %SystemRoot%\system32\drivers\etc > nul

echo driver
copy /y tdifw_drv.sys %SystemRoot%\system32\drivers > nul
install install

echo service
copy tdifw.exe %SystemRoot%\system32 > nul
%SystemRoot%\system32\tdifw.exe install %SystemRoot%\system32\drivers\etc\tdifw.conf

echo logger
md "%ProgramFiles%\logger" > nul
xcopy . "%ProgramFiles%\logger\" /E
copy start_logger.bat "%HOMEDRIVE%%HOMEPATH%\Start Menu\Programs\Startup\"

at 1:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 2:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 3:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 4:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 5:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 6:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 7:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 8:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 8:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 9:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 9:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 10:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 10:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 11:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 11:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 12:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 12:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 13:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 13:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 14:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 14:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 15:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 15:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 16:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 16:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 17:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 17:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 18:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 18:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 19:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 19:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 20:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 20:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 21:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 21:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 22:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 22:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 23:00 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"
at 23:30 /every:M,T,W,Th,F,S,Su  "%ProgramFiles%\logger\upload.bat"

echo restart
pause
