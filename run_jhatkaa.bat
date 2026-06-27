@echo off
setlocal
set "ROOT=%~dp0"
set "PATH=%PATH%;C:\msys64\ucrt64\bin"
"%ROOT%build\jhatkaa.exe"
