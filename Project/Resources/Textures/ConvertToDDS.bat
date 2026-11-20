@echo off
rem ============================================================
rem  Convert png/jpg/tga/bmp under this folder to BC7 DDS (with mips)
rem  Output: <source folder>\Cache\<name>_srgb.dds / <name>_lin.dds
rem  (same naming as TextureLoader::LoadFromFileCached)
rem ============================================================
setlocal enabledelayedexpansion

set TEXCONV=%~dp0..\..\Externals\DirectXTex\texconv.exe
set ROOT=%~dp0

if not exist "%TEXCONV%" (
  echo texconv.exe not found: %TEXCONV%
  pause
  exit /b 1
)

set /a OK=0
set /a FALLBACK=0
set /a FAIL=0

for /r "%ROOT%" %%F in (*.png *.jpg *.jpeg *.tga *.bmp) do (
  rem Skip files inside Cache folders
  echo %%~dpF | findstr /i "\\Cache\\" >nul
  if errorlevel 1 (
    if not exist "%%~dpFCache" mkdir "%%~dpFCache"

    rem Data textures (normal/roughness etc.) are converted as linear
    set NAME=%%~nF
    set LINEAR=0
    for %%K in (_bump _normal _nrm _ddn _spec _rough _metal _ao _mask _height _disp) do (
      echo !NAME! | findstr /i "%%K" >nul && set LINEAR=1
    )

    if !LINEAR!==1 (
      set FMT=BC7_UNORM
      set RAWFMT=R8G8B8A8_UNORM
      set SRGBOPT=
      set SUFFIX=_lin
    ) else (
      set FMT=BC7_UNORM_SRGB
      set RAWFMT=R8G8B8A8_UNORM_SRGB
      set SRGBOPT=-srgb
      set SUFFIX=_srgb
    )

    echo [!SUFFIX!] %%F
    "%TEXCONV%" -nologo -y -m 0 -f !FMT! !SRGBOPT! -sx !SUFFIX! -o "%%~dpFCache" "%%F" > "%TEMP%\texconv_log.txt" 2>&1
    set BCFAIL=!errorlevel!
    rem texconv only warns (exit 0) when size is not a multiple of 4, which D3D12 cannot use
    findstr /c:"multiple of 4" "%TEMP%\texconv_log.txt" >nul && set BCFAIL=1
    if !BCFAIL! neq 0 (
      rem BC compression failed or unusable - save uncompressed
      "%TEXCONV%" -nologo -y -m 0 -f !RAWFMT! !SRGBOPT! -sx !SUFFIX! -o "%%~dpFCache" "%%F" >nul
      if errorlevel 1 (
        echo   FAILED
        set /a FAIL+=1
      ) else (
        echo   saved uncompressed
        set /a FALLBACK+=1
      )
    ) else (
      set /a OK+=1
    )
  )
)

echo.
echo Done. BC7: %OK%  Uncompressed: %FALLBACK%  Failed: %FAIL%
pause
