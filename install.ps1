$PROJECT_NAME = "CSOL24H"
$PROJECT_PATH = (Get-Location).ToString() -replace("\\", "/")

$SOURCE_PATH = Join-Path $PROJECT_PATH "source"
$BUILD_PATH = Join-Path $PROJECT_PATH "build"
$EXEC_NAME = -join($PROJECT_NAME, ".LUA")

foreach ($file in Get-ChildItem $SOURCE_PATH)
{
    # Write-Host $file.FullName
    (Get-Content -Path $file) -replace("local\s+PATH\s*=\s*`".*`"\s+", "local PATH = `"$PROJECT_PATH/`"") | Out-File -FilePath $file.FullName
}

New-Item -Type Directory -Path $BUILD_PATH -Force

Out-File -FilePath "$BUILD_PATH/$EXEC_NAME" -InputObject "local PATH = `"$PROJECT_PATH/`""
Out-File -Append -FilePath "$BUILD_PATH/$EXEC_NAME" -InputObject "dofile(PATH .. `"source/main.lua`")"

Set-Location $PROJECT_PATH