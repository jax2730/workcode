@echo off
copy /b CMakeLists.txt +,, > NUL

cd EchoCommon
copy /b CMakeLists.txt +,, > NUL

cd ..\EchoCore
copy /b CMakeLists.txt +,, > NUL

cd ..\EchoEngine
copy /b CMakeLists.txt +,, > NUL

cd ..\EchoPhysics
copy /b CMakeLists.txt +,, > NUL

cd ..\EchoComponent
copy /b CMakeLists.txt +,, > NUL

cd ..\EchoGame
copy /b CMakeLists.txt +,, > NUL

cd ..