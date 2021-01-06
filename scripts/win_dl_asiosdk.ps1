##############################################################################
## This script will auto-download and unpack the latest Steinberg ASIO SDK. ##
## Assumes CWD is at the root level (eg: same location as CMakeLists)       ##
##############################################################################

# project paths
$LIB_DIR="libs"
$SDK_DIRNAME="asiosdk"
$SDK_LOCATION="$LIB_DIR\$SDK_DIRNAME"

# download params
$ASIO_SDK_URL="https://www.steinberg.net/asiosdk"
$SDK_DOWNLOAD_FILE="asiosdk.zip"

Write-Output "-- Checking for Steinberg ASIO SDK..."
if (-not (Test-Path $SDK_LOCATION))
{
	if (-not (Test-Path $SDK_DOWNLOAD_FILE))
	{
		Write-Output "--   Downloading Steinberg ASIO SDK..."
		Invoke-WebRequest -Uri $ASIO_SDK_URL -UseBasicParsing -OutFIle $SDK_DOWNLOAD_FILE
	} else {
		Write-Output "--   Found $SDK_DOWNLOAD_FILE from previous download."
	}
	Write-Output "--   Extracting to $SDK_LOCATION..."
	Expand-Archive $SDK_DOWNLOAD_FILE $LIB_DIR
	Set-Location $LIB_DIR
	
	# SDK zip has it in a timestamped directory name. This strips it off.
	(Get-ChildItem -Filter "$SDK_DIRNAME*")[0] | Rename-Item -NewName { $_.Name -Replace $_.Name, $SDK_DIRNAME }
} else {
	Write-Output "--   Steinberg ASIO SDK present in $SDK_LOCATION."
}