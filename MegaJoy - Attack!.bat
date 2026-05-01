@echo off
REM ============================================================
REM  AES Controller - Legacy Gamepad Firmware Flasher
REM  For Arduino Mega 2560
REM ============================================================

if not exist ATmega8u2Code\HexFiles\batchisp.exe (
    echo.
    echo ERROR: ATmega8u2Code\HexFiles\batchisp.exe not found!
    echo.
    echo Did you move this .bat file?
    echo Did you delete something in the ATmega8u2Code folder?
    echo.
    echo Press any key to exit...
    goto EXIT
)

echo.
echo ============================================================
echo   AES Controller - Gamepad Firmware Flasher (Legacy)
echo ============================================================
echo.
echo IMPORTANT: Before putting Arduino in DFU mode:
echo  1. Connect Arduino to PC via USB
echo  2. Short the 2 pins on ICSP header (RESET + GND)
echo  3. Wait 1 second and release the pins
echo  4. Arduino should be in DFU mode
echo.
echo Press any key to continue...
pause > nul

echo.
echo Flashing gamepad firmware...
echo.

cd ATmega8u2Code\HexFiles

echo [1/2] Trying Arduino Mega 2560 R1/R2 (at90usb82)...
@echo on
batchisp -device at90usb82 -hardware usb -operation erase f memory flash blankcheck loadbuffer "MegaJoy.hex" program verify start reset 1024
@echo off

if %errorlevel% EQU 0 goto SUCCESS

echo.
echo [2/2] Trying Arduino Mega 2560 R3 (atmega16u2)...
echo.
@echo on
batchisp -device atmega16u2 -hardware usb -operation erase f memory flash blankcheck loadbuffer "MegaJoy.hex" program verify start reset 1024
@echo off

if %errorlevel% NEQ 0 (
    echo.
    echo ============================================================
    echo   ERROR: Firmware was NOT loaded!
    echo ============================================================
    echo.
    echo Possible reasons:
    echo  - Atmel FLIP not installed: http://www.atmel.com/tools/FLIP.aspx
echo  - Arduino not connected
echo  - Arduino not in DFU mode
echo  - Driver issue
echo.
    echo Press any key to exit...
    goto EXIT
)

:SUCCESS
echo.
echo ============================================================
echo   SUCCESS! Gamepad firmware loaded.
echo ============================================================
echo.
echo Now you need to:
echo  1. Unplug Arduino USB cable
echo  2. Wait 3 seconds
echo  3. Plug USB cable back in
echo  4. Windows will show it as a joystick
echo.

:EXIT
pause > nul
