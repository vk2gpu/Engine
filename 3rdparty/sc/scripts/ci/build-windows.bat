@echo on

set GENERATOR=%TOOLSET%
if "%2" NEQ "" set GENERATOR=%GENERATOR% %2

mkdir build-%1%2
cd build-%1%2

cmake -G "%GENERATOR%" ..
if %errorlevel% neq 0 exit /b %errorlevel%

msbuild /m "/p:Configuration=%1" "sc.sln" /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
if %errorlevel% neq 0 exit /b %errorlevel%

"bin\%1\sc_tests.exe" --reporter console
if %errorlevel% neq 0 exit /b %errorlevel%

cd ..
