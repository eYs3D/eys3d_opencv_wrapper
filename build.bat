@set support=
@if "%~1"=="1" goto build
@if "%~1"=="2" goto setvar
set "item1=1. OpenCV(x64)-3.4.2"
set "item2=2. OpenCV(x64)-3.4.2 support QT^&OpenGL"
@echo Project list : 
@echo ==================
@echo %item1%
@echo %item2%
@echo ==================
@echo.
@set /p answer=Please select project:
@if /i "%answer:~,1%" EQU "1" goto build
@if /i "%answer:~,1%" EQU "2" goto setvar
@goto exit
:setvar
@set support=-DSupport=opengl
:build
rmdir build /S/Q
mkdir build
cd build
cmake .. -G "Visual Studio 15 2017" -A x64 %support%
msbuild /P:Configuration=Release eys3d_opencv.vcxproj
cd ..
:exit
