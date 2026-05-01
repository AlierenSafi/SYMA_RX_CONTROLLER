@echo off
REM ============================================================
REM  AES Controller - ATmega16U2 Gamepad Firmware Flasher
REM  Arduino Mega 2560 icin
REM ============================================================

if not exist ATmega8u2Code\HexFiles\batchisp.exe (
    echo.
    echo HATA: ATmega8u2Code/HexFiles/batchisp.exe bulunamadi!
    echo.
    echo Bu .bat dosyasini tasidiniz mi?
    echo ATmega8u2Code klasorunu sildiniz mi?
    echo.
    echo Cikmak icin bir tusa basin...
    goto EXIT
)

echo.
echo ============================================================
echo   AES Controller - Gamepad Firmware Yukleyici
echo ============================================================
echo.
echo ONEMLI: Arduino'yu DFU moduna almadan once:
echo  1. Arduino'yu USB ile bilgisayara baglayin
echo  2. ICSP headerindaki 2 pini kisa devre yapin (RESET + GND)
echo  3. 1 saniye bekleyin ve pinleri ayirin
echo  4. Arduino DFU modunda olmali
echo.
echo Devam etmek icin bir tusa basin...
pause > nul

echo.
echo Firmware yukleniyor...
echo.

cd ATmega8u2Code\HexFiles

echo [1/2] Arduino Mega 2560 R1/R2 deneniyor (at90usb82)...
@echo on
batchisp -device at90usb82 -hardware usb -operation erase f memory flash blankcheck loadbuffer "AES_Controller.hex" program verify start reset 1024
@echo off

if %errorlevel% EQU 0 (
    goto SUCCESS
)

echo.
echo [2/2] Arduino Mega 2560 R3 deneniyor (atmega16u2)...
echo.
@echo on
batchisp -device atmega16u2 -hardware usb -operation erase f memory flash blankcheck loadbuffer "AES_Controller.hex" program verify start reset 1024
@echo off

if %errorlevel% NEQ 0 (
    echo.
    echo ============================================================
    echo   HATA: Firmware yuklenemedi!
    echo ============================================================
    echo.
    echo Olası nedenler:
    echo  - Atmel FLIP yuklu degil: http://www.atmel.com/tools/FLIP.aspx
echo  - Arduino bagli degil
echo  - Arduino DFU modunda degil
echo  - Surucu sorunu
echo.
    echo Cikmak icin bir tusa basin...
    goto EXIT
)

:SUCCESS
echo.
echo ============================================================
echo   BASARILI! AES Controller firmware yuklendi.
echo ============================================================
echo.
echo Simdi yapmaniz gerekenler:
echo  1. Arduino'nun USB kablosunu cikarin
echo  2. 3 saniye bekleyin
echo  3. USB kablosunu tekrar takin
echo  4. Windows'ta gamepad olarak gorunecek
echo.

:EXIT
pause > nul
