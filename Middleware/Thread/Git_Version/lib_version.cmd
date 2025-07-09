@echo off 

echo Start to get the git hash information....
if "%1"=="" (
  echo Usage: %0 ^<header_file_name^>
  goto :eof
)

for /F "usebackq" %%v in (`"git describe --always --abbrev=8 --exclude "*""`) DO (
  if exist %1.new del %1.new
  echo #ifndef __GIT_VERSION_H__> %1.new
  echo #define __GIT_VERSION_H__>> %1.new
  echo #define LIB_VERSION "%%v">> %1.new
  echo #define BUILD_DATE "BDATE:%DATE:~0,10% %TIME:~0,8%">> %1.new
  echo #endif>> %1.new
  echo version:%%v
)
if not exist %1 (
  move %1.new %1
) else (
    if "0" == "%%c" (
      echo Use original %1
      del %1.new
    ) else (
      echo Use new %1
      del %1
      move %1.new %1
    )
)
