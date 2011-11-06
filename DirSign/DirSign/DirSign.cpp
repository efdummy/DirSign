//____________________________________________________________________________________________
//
//                 
//             SIGNATURE DE REPERTOIRE
//             (pour détecter une modification dans un répertoire sans mettre en place une surveillance avec un file system watcher)
//
//             - Signature d'une arborescence de répertoires
//               (calculée d'après les tailles, dates et quantité de répertoires et fichiers)
//             - Contrôle de signature d'une arborescence de répertoires
//			   
//             DIRECTORY SIGNATURE (to detect an update in a directory tree without filesystem watcher)
//             - Sign a directory tre
//               (calculated on number, size and last update date of dirs and files
//             - Check a signature againts a directory tree
//
//
//             Eric Fullenbaum - Octobre 2004 - Novembre 2011
//____________________________________________________________________________________________


#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <io.h>

#define LINE "\n_____________________________________________________________________________\n"

#define FILE_NAME_SIZE   260
#define MAX_LINE_SIZE  32768
#define MAX_DIAG_SIZE   2048

//_______ Error

#define RC_NB_PARAMS           1
#define RC_NB_PARAMS_MSG       L"Incorrect number of parameters"
#define RC_BADOPTION           2
#define RC_BADOPTION_MSG       L"Option unknown"
#define RC_DIRNOTFOUND         3
#define RC_DIRNOTFOUND_MSG     L"Directory not found"
#define RC_BADSIGNATURE        4
#define RC_BADSIGNATURE_MSG    L"Bad signature (not an integer)"

//_______ Options

#define OPT_SIGN			L"-sign"
#define OPT_CHECK			L"-check"

//_______ Process files recursively or not, filter last matching pattern

#define FLAG_RECURSE	1
#define FLAG_NORECURSE  0

#define BOOL_DISPLAY	1
#define BOOL_NODISPLAY  0

//_______ Doc (éèêàù=‚Šˆ…— en ascii)
#define PROGNAME "DirSign"
void usage(_TCHAR *szProgName)
{
	printf(LINE);
	printf("\n%s evaluates or checks directory signature.", PROGNAME);
	printf("\n%s is used to check if something in a directory tree has changed", PROGNAME);
	printf("\n        (a file date or a file size or a new or missing file).", PROGNAME);
	printf("\n        Use it in scenario where you can't install a file system watcher.", PROGNAME);
	printf("\nSyntax :");
	printf("\n       %s <path>", PROGNAME);
	printf("\n       %s -sign <path>", PROGNAME);
	printf("\n       %s -check <signature> <path>", PROGNAME);
	printf("\nSamples :");
	printf("\n       C:\\> %s c:\\temp", PROGNAME);
	printf("\n       Outputs recursively the signature process for c:\\temp");
	printf("\n       with intermediate counters.");
	printf("\n       C:\\> %s -sign c:\\temp", PROGNAME);
	printf("\n       Outputs the signature of c:\\temp");
	printf("\n       Signature is also returned in errorlevel but overflow is possible.");
	printf("\n       C:\\> %s -check 65412345 c:\\temp", PROGNAME);
	printf("\n       Outputs 1 if the signature of the directory is OK or 0 if not");
	printf("\n       The result is also returned in errorlevel.");
	printf("\n");
	printf("\n%s Eric Fullenbaum - 2004 - 2011", PROGNAME);
	printf(LINE);
}

//_______ File description structure

typedef struct _finddata_t _finddata_t_struct;

//_______ Counters
struct Counters
{
	unsigned long ulNumberOfDirectories;
	unsigned long ulNumberOfFiles      ;
	unsigned long ulFilesTotalSize     ;
	unsigned long ulSumOfDirDates      ;
	unsigned long ulSumOfFiledates     ;
};

//_______ True if uiAttrib is a directory attrib

unsigned short boolDirAttrib(unsigned int uiAttrib)
{
//	return (uiAttrib==( (_A_SUBDIR) || (_A_SUBDIR&&_A_RDONLY) || (_A_SUBDIR&&_A_ARCH) ));
	return (_A_SUBDIR & uiAttrib);
}

// Signature is calculated on file sizes, dates, number of files, number of directories and directories dates
unsigned long signature(Counters *counters)
{
	return 
		counters->ulNumberOfDirectories+
		counters->ulNumberOfFiles+
		counters->ulFilesTotalSize+
		counters->ulSumOfDirDates+
		counters->ulSumOfFiledates;
}

_TCHAR szPreviousPath[FILE_NAME_SIZE];
// Trace counters to screen output
void trace(_TCHAR * szPath, Counters *counters)
{
	if (_wcsicmp(szPath, szPreviousPath)==0)
	{
		wprintf(L"#d:%u #f:%u s:%u fdates:%u ddates:%u sign:%u\n",
			counters->ulNumberOfDirectories,
			counters->ulNumberOfFiles,
			counters->ulFilesTotalSize,
			counters->ulSumOfDirDates,
			counters->ulSumOfDirDates,
			signature(counters));
	}
	else
	{
		wcscpy(szPreviousPath, szPath);
		wprintf(L"%s\n#d:%u #f:%u s:%u fdates:%u ddates:%u sign:%u\n",
			szPath,
			counters->ulNumberOfDirectories,
			counters->ulNumberOfFiles,
			counters->ulFilesTotalSize,
			counters->ulSumOfDirDates,
			counters->ulSumOfDirDates,
			signature(counters));
	}
}

//_______ Process File
void processFile(_TCHAR * szPath,  _wfinddata_t *fileinfo, int boolDisplay, Counters *counters)
{
	counters->ulNumberOfFiles++;
	counters->ulFilesTotalSize+=fileinfo->size;
	counters->ulSumOfFiledates+=(* fileinfo).time_write;
	if (boolDisplay) trace(szPath, counters);
}

//_______ Process directory
void processDir(_TCHAR * szPath, int boolDisplay, struct _wfinddata_t *fileinfo, Counters *counters)
{
	counters->ulNumberOfDirectories++;
	counters->ulSumOfDirDates+=(* fileinfo).time_write;
	if (boolDisplay) trace(szPath, counters);
}

//_______ Enumerate directories according to the options 
//_______ (recursively or not, with pattern matching or not)

int enumDirectoriesAndProcessEach(_TCHAR *path, int flRecurse, int boolDisplay, Counters *counters)
{
	struct _wfinddata_t fileinfo;
	long   handle;
	int    rc, boolNotCurrentDir;
	_TCHAR   dirSpec[260];

	rc=0;

	// Start enum subdir
	wcscpy(dirSpec, path);
	wcscat(dirSpec, L"\\*");

	handle = _wfindfirst(dirSpec, &fileinfo);
	if (handle!=-1L) {
		if (counters->ulSumOfDirDates==0) { counters->ulSumOfDirDates+=fileinfo.time_write;
								  //printf("%s %d\n", fileinfo.name, fileinfo.time_write);
		}
		// Equal to zero if current dir
		boolNotCurrentDir=wcscmp(fileinfo.name, L".");
		if (boolNotCurrentDir) {

			// Process first subdir
			if (boolDirAttrib(fileinfo.attrib)) {
				wcscpy(dirSpec, path);
				wcscat(dirSpec, L"\\");
				wcscat(dirSpec, fileinfo.name);
				processDir(dirSpec, boolDisplay, &fileinfo, counters);
				if (flRecurse==FLAG_RECURSE)
					enumDirectoriesAndProcessEach(dirSpec, FLAG_RECURSE, boolDisplay, counters);
			} else {
				processFile(path, &fileinfo, boolDisplay, counters);
			}
		}
		while(_wfindnext(handle, &fileinfo )==0) {
				boolNotCurrentDir=wcscmp(fileinfo.name, L"..");
				// Process all other subdirs
				if (boolNotCurrentDir) {
					if (boolDirAttrib(fileinfo.attrib)) {
						wcscpy(dirSpec, path);
						wcscat(dirSpec, L"\\");
						wcscat(dirSpec, fileinfo.name);
						processDir(dirSpec, boolDisplay, &fileinfo, counters);
						if (flRecurse==FLAG_RECURSE)
							enumDirectoriesAndProcessEach(dirSpec, FLAG_RECURSE, boolDisplay, counters);
					} else {
						processFile(path, &fileinfo, boolDisplay, counters);
					}
				}
		}
	// Stop enum
	_findclose(handle);
	}
	else rc=1;

	return rc;
}

unsigned long getDirDate(char *szDirPath)
{
	struct _finddata_t fileinfo;
	long   handle;
	int    rc;

	rc=0;

	handle = _findfirst(szDirPath, &fileinfo);
	if (handle!=-1L) {
		rc=fileinfo.time_write;
	} else {
		rc=1;
	}
	return rc;
}

//__________________________________________ main __________________________________________
unsigned long _tmain(int argc, _TCHAR* argv[])
{
	int rc;
	unsigned long rv;

	rc=0;

	//_______ Counters
    struct Counters counters;


	counters.ulFilesTotalSize=0;
	counters.ulSumOfFiledates=0;
	counters.ulNumberOfDirectories=0;
	counters.ulNumberOfFiles=0;
	counters.ulSumOfDirDates=0;

	switch(argc) {
	case 1:
		// No params, display doc
		usage(argv[0]);
		break;
	case 2:
		// One param : display all dir
		rc=(unsigned long)enumDirectoriesAndProcessEach(argv[1], FLAG_RECURSE, BOOL_DISPLAY, &counters);
		if (rc==1)
			rc=RC_DIRNOTFOUND;
		else
			rv=signature(&counters);
		break; 
	case 3:
		// Two parameters : sign
		if (_wcsicmp(argv[1], OPT_SIGN)==0)
		{
			rc=(unsigned long)enumDirectoriesAndProcessEach(argv[2], FLAG_RECURSE, BOOL_NODISPLAY, &counters);
			if (rc==1)
				rc=RC_DIRNOTFOUND;
			else
				rv=signature(&counters);
			break;
		}
		rc=RC_BADOPTION;
		break;
	case 4:
		// Three parameters :  check
		if (_wcsicmp(argv[1], OPT_CHECK)==0)
		{
			unsigned long ulSignature=_wtol(argv[2]);
			if (ulSignature==0)
				rc=RC_BADSIGNATURE;
			else
			{
				rc=(unsigned long)enumDirectoriesAndProcessEach(argv[3], FLAG_RECURSE, BOOL_NODISPLAY, &counters);
				if (rc==1)
					rc=RC_DIRNOTFOUND;
				else
				{
					rv=0;
					if (ulSignature==signature(&counters)) rv=1;
				}
			}
			break;
		}
		rc=RC_BADOPTION;
		break;
	default:
		// bad # of params
		rc=RC_NB_PARAMS;
		break;

	}
	if (rc==RC_NB_PARAMS) { wprintf(L"%s - %d\n", RC_NB_PARAMS_MSG, argc-1); }
	if (rc==RC_BADOPTION) { wprintf(L"%s %s with %d parameters\n", RC_BADOPTION_MSG, argv[1], argc-2); }
	if ((rc==RC_DIRNOTFOUND)&&(argc==2)) { wprintf(L"%s %s\n", RC_DIRNOTFOUND_MSG, argv[1]); }
	if ((rc==RC_DIRNOTFOUND)&&(argc==3)) { wprintf(L"%s %s\n", RC_DIRNOTFOUND_MSG, argv[2]); }
	if ((rc==RC_DIRNOTFOUND)&&(argc==4)) { wprintf(L"%s %s\n", RC_DIRNOTFOUND_MSG, argv[3]); }
	if ((rc==RC_BADSIGNATURE)&&(argc==4)) { wprintf(L"%s %s\n", RC_BADSIGNATURE_MSG, argv[2]); }
	if ((argc!=1)&&(rc==0)) wprintf(L"%u\n", rv);
	return rc;
}
