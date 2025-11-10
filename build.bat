@echo off
setlocal EnableDelayedExpansion

REM Wrapper semplice per compilare il progetto in automatico (doppio clic)
REM - Compila Release x64 del target principale e mostra output finale

call "%~dp0scripts\build-windows.bat" --reconfigure -c Release -t HairlessMidiSerial --pause

endlocal