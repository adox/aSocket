@echo off

echo.
echo Simple Arduino make script.
echo.
echo Author: Adrian Brzezinski
echo Contact: adrb@wp.pl iz0@poczta.onet.pl
echo.

set arduino_sketch_path=
set arduino_external_libs=
if "%arduino_sketch_path%" == "" set arduino_sketch_path=%CD%

set arduino_ide_path=C:\f\arduino-1.0-windows\arduino-1.0
set arduino_ide_version=100
set arduino_core_path=%arduino_ide_path%\hardware\arduino\cores\arduino

set avr_includes= -I%arduino_ide_path%\hardware\tools\avr\avr\include\avr  -I%arduino_core_path% -I%arduino_sketch_path%
set avr_mcu=atmega328p
set avr_f_cpu=16000000L
set avr_flash_size=30720
set avr_sram_size=2048

set avr_bin_path=%arduino_ide_path%\hardware\tools\avr\bin
set avr_etc_path=%arduino_ide_path%\hardware\tools\avr\etc
set avr_gcc=%avr_bin_path%\avr-gcc -c -Os -w -mcall-prologues -ffunction-sections -fdata-sections -mmcu=%avr_mcu% -DF_CPU=%avr_f_cpu% -DARDUINO=%arduino_ide_version%
set avr_g++=%avr_bin_path%\avr-g++ -c -Os -w -mcall-prologues -fno-exceptions -ffunction-sections -fdata-sections -mmcu=%avr_mcu% -DF_CPU=%avr_f_cpu% -DARDUINO=%arduino_ide_version%
set avr_out_path=.\build

set avrdude_port=COM2
set avrdude_protocol=stk500v1
set avrdude_speed=57600

setlocal EnableExtensions
setlocal EnableDelayedExpansion

echo Cleaning up...
rd /s /q %avr_out_path% 2>nul
md %avr_out_path%

echo.
echo Building core.
echo.

for %%s in (%arduino_core_path%\*.c %arduino_core_path%\*.cpp) do (

	echo Compiling file %%~nxs
	
	if %%~xs == .c (
		%avr_gcc% -I%arduino_core_path% %%s -o%avr_out_path%\%%~nxs.o
	)
	
	if %%~xs == .cpp (
		%avr_g++% -I%arduino_core_path% %%s -o%avr_out_path%\%%~nxs.o
	)
)

echo.
echo Building selected external libs.
echo ( %arduino_external_libs% )
echo.

set arduino_extlib_inc=
for %%l in (%arduino_external_libs%) do (
	set arduino_extlib_inc=!arduino_extlib_inc! -I%arduino_ide_path%\libraries\%%l
)

for %%l in (%arduino_external_libs%) do (
	set arduino_extlib_path=%arduino_ide_path%\libraries\%%l

	for /f "tokens=*" %%s in ('dir /s /b !arduino_extlib_path!\*.c !arduino_extlib_path!\*.cpp') do (
		
		if not exist "%avr_out_path%\%%~nxs.o" (
		
			echo Compiling file %%~nxs
	
			if %%~xs == .c (
				%avr_gcc% %avr_includes% %arduino_extlib_inc% %%s -o%avr_out_path%\%%~nxs.o
			)
	
			if %%~xs == .cpp (
				%avr_g++% %avr_includes% %arduino_extlib_inc% %%s -o%avr_out_path%\%%~nxs.o
			)
		)
	)
)

REM Retriving sketch name from path
set _app=%arduino_sketch_path%
:nexchar
set _t=%_app:~-1%
set _app=%_app:~0,-1%
if %_t% == \ goto :setsketchname
if %_t% == / goto :setsketchname
goto :nexchar
:setsketchname
set arduino_sketch_name=!arduino_sketch_path:%_app%\=!

echo.
echo Building sketch.
echo ( %arduino_sketch_name% )
echo.

if exist %arduino_sketch_path%\%arduino_sketch_name%.pde (

	echo Processing file %arduino_sketch_name%.pde

	echo #include "WProgram.h" >> %avr_out_path%\%arduino_sketch_name%.cpp
	type %arduino_sketch_path%\%arduino_sketch_name%.pde >> %avr_out_path%\%arduino_sketch_name%.cpp
)

for %%s in (%arduino_sketch_path%\*.pde %arduino_sketch_path%\*.c %arduino_sketch_path%\*.cpp) do (

	if %%~xs == .pde (
		if not %%~nxs == %arduino_sketch_name%.pde (

			echo Processing file %%~nxs

			type %%s >> %avr_out_path%\%arduino_sketch_name%.cpp
		)

	) else (

		echo Compiling file %%~nxs
	
		if %%~xs == .c (
			%avr_gcc% %avr_includes% %arduino_extlib_inc% %%s -o%avr_out_path%\%%~nxs.o
		)
	
		if %%~xs == .cpp (
			%avr_g++% %avr_includes% %arduino_extlib_inc% %%s -o%avr_out_path%\%%~nxs.o
		)
	)
)

if exist %avr_out_path%\%arduino_sketch_name%.cpp (

	echo.
	echo Compiling pde files

	%avr_g++% %avr_includes% %arduino_extlib_inc% %avr_out_path%\%arduino_sketch_name%.cpp -o%avr_out_path%\%arduino_sketch_name%.o
)

echo.
echo Linking all
echo.

%avr_bin_path%\avr-ar rcs %avr_out_path%\%arduino_sketch_name%.a %avr_out_path%\*.o
%avr_bin_path%\avr-gcc -Os -Wl,--gc-sections -mmcu=%avr_mcu% -o %avr_out_path%\%arduino_sketch_name%.elf %avr_out_path%\%arduino_sketch_name%.a -L%avr_out_path% -lm

%avr_bin_path%\avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 %avr_out_path%\%arduino_sketch_name%.elf %avr_out_path%\%arduino_sketch_name%.eep

%avr_bin_path%\avr-objcopy -O ihex -R .eeprom %avr_out_path%\%arduino_sketch_name%.elf %avr_out_path%\%arduino_sketch_name%.hex

set /a hex_size=0
set /a elf_sram=0
if exist %avr_out_path%\%arduino_sketch_name%.hex (

	for /f "skip=3 tokens=1-3" %%a in ('%avr_bin_path%\avr-size -A %avr_out_path%\%arduino_sketch_name%.hex') do set /a hex_size=%%b

	echo Binary sketch size: !hex_size! bytes, of a %avr_flash_size% byte maximum.
	
	for /f "skip=2 tokens=1-3" %%a in ('%avr_bin_path%\avr-size -A %avr_out_path%\%arduino_sketch_name%.elf') do (
	
		if %%a == .data (
			set /a elf_sram+=%%b
		)

		if %%a == .bss (
			set /a elf_sram+=%%b
		)
	)

	echo Static memory usage: !elf_sram! bytes, of a %avr_sram_size% byte maximum.
	if !elf_sram! geq %avr_sram_size% echo WARNING! Oversized SRAM!

	if !hex_size! leq %avr_flash_size% (

		echo.
		set /p upload=Upload sketch [y/n]?

		if !upload! == y (
			%avr_bin_path%\avrdude -C%avr_etc_path%\avrdude.conf -p%avr_mcu% -c%avrdude_protocol% -P\\.\%avrdude_port% -b%avrdude_speed% -D -u -V -Uflash:w:"%avr_out_path%\%arduino_sketch_name%.hex":i
		)
	) else (
		echo WARNING! Oversized sketch file!
	)
)

echo All done.
pause
