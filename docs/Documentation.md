# DirSign evaluates or checks directory signature
DirSign is used to check if something in a directory tree has changed
(a file date or a file size or a new or missing file).
You use DirSign in scenario where you can't install a file system watcher.

## Syntax
* DirSign
* DirSign <path>
* DirSign -sign <path>
* DirSign -check <signature> <path>

## Samples
C:\> DirSign
Dirsign.exe without parameter will display documentation.

C:\> DirSign c:\temp
Outputs recursively the signature process for c:\temp with intermediate counters.

C:\> DirSign -sign c:\temp
Outputs the signature of c:\temp to the screen. Signature is also returned in errorlevel but overflow is possible.

C:\> DirSign -check 65412345 c:\temp
Outputs 1 if the signature of the directory is OK or 0 if not. The result (1 or 0) is also returned in errorlevel.

## Cmd script integration sample
@DirSign.exe -sign c:\temp >.\previous.sign

@echo Do some change or not in c:\temp
@pause

@DirSign.exe -sign c:\temp >.\new.sign

@set errorlevel=
@fc .\new.sign .\previous.sign >nul
@set rc=%errorlevel%

@if "%rc%"=="0" echo No change in c:\temp
@if "%rc%"=="1" echo A change occurs in c:\temp

## Returned codes
The returned codes in errorlevel are the following :
* 2 "Display doc"
* 3 "Incorrect number of parameters"
* 4 "Option unknown"
* 5 "Directory not found"
* 6 "Bad signature (not a number)"

## Details
* On reasonable size directory tree (for example 5 000 directories, 50 000 files), dirsign.exe will sign the tree structure in a few seconds.
* In some rare occasion, dirsign will miss a change in the tree (in the case of an infrequent compensation between number of directories, number of files, size of files, update time of files and update time of directories). If the changes in the directory are always with new files (more directories, more files, more recent update date for files or directories or different number of files), dirsign will always catch the difference.

