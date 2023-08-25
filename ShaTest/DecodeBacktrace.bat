:: Decodes an exception backtrace in the Lumos32i program. 
:: Need to setup ESP32 esp-idf environment first. 
:: https://jsimoesblog.wordpress.com/2022/11/04/decoding-esp32-back-trace/
:: xtensa-esp32s3-elf-addr2line arguments:
::  @<file>                Read options from <file>
::  -a --addresses         Show addresses
::  -b --target=<bfdname>  Set the binary file format
::  -e --exe=<executable>  Set the input file name (default is a.out)
::  -i --inlines           Unwind inlined functions
::  -j --section=<name>    Read section-relative offsets instead of addresses
::  -p --pretty-print      Make the output easier to read for humans
::  -s --basenames         Strip directory names
::  -f --functions         Show function names
::  -C --demangle[=style]  Demangle function names
::  -R --recurse-limit     Enable a limit on recursion whilst demangling.  [Default]
::  -r --no-recurse-limit  Disable a limit on recursion whilst demangling
::  -h --help              Display this information
::  -v --version           Display the program's version

:: Usage:
:: DecodeBackTrace.bat 0x403761b2:0x3fcba220 0x403816fd:0x3fcba240 0x40388b69:0x3fcba260 0x403...
@ECHO OFF

for /f "tokens=1,* delims= " %%a in ("%*") do set ALL_BUT_FIRST=%%b
xtensa-esp32s3-elf-addr2line -apCfire "build\VisualGDB\Debug\hello_world.elf" %ALL_BUT_FIRST%
