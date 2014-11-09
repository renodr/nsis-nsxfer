
#include "main.h"
#include "utils.h"
#include "queue.h"
#include "gui.h"

HINSTANCE g_hInst = NULL;
BOOL g_bInitialized = FALSE;
QUEUE g_Queue = { 0 };


/// Forward declarations
UINT_PTR __cdecl NsisMessageCallback( enum NSPIM iMessage );


//++ PluginInit
BOOL PluginInit()
{
	BOOL bRet = TRUE;
	if ( !g_bInitialized ) {

		TRACE( _T( "PluginInit\n" ) );

		UtilsInitialize();
		QueueInitialize( &g_Queue, _T("MAIN"), 2 );

		if ( TRUE ) {
			g_bInitialized = TRUE;
		} else {
			bRet = FALSE;
		}
	}
	return bRet;
}


//++ PluginUninit
BOOL PluginUninit( _In_ BOOLEAN bForced )
{
	BOOL bRet = FALSE;
	if ( g_bInitialized ) {

		TRACE( _T( "PluginUninit(Forced:%u)\n" ), (ULONG)bForced );

		QueueDestroy( &g_Queue, bForced );
		UtilsDestroy();

		g_bInitialized = FALSE;
		bRet = TRUE;
	}
	return bRet;
}


//++ NsisMessageCallback
UINT_PTR __cdecl NsisMessageCallback( enum NSPIM iMessage )
{
	switch ( iMessage )
	{
	case NSPIM_UNLOAD:
		TRACE( _T( "NSPIM_UNLOAD\n" ) );
		PluginUninit( FALSE );
		break;

	case NSPIM_GUIUNLOAD:
		TRACE( _T( "NSPIM_GUIUNLOAD\n" ) );
		break;
	}
	return 0;
}


//++ Transfer
EXTERN_C __declspec(dllexport)
void __cdecl Transfer(
	HWND   parent,
	int    string_size,
	TCHAR   *variables,
	stack_t **stacktop,
	extra_parameters *extra
	)
{
	LPTSTR psz = NULL;
	ULONG iPriority = DEFAULT_VALUE;
	LPTSTR pszMethod = NULL;
	LPTSTR pszUrl = NULL, pszFile = NULL;
	ITEM_LOCAL_TYPE iLocalType = ITEM_LOCAL_NONE;
	LPTSTR pszHeaders = NULL;
	LPVOID pData = NULL;
	ULONG iDataSize = 0;
	ULONG iTimeoutConnect = DEFAULT_VALUE, iTimeoutReconnect = DEFAULT_VALUE;
	ULONG iOptConnectRetries = DEFAULT_VALUE, iOptConnectTimeout = DEFAULT_VALUE, iOptRecvTimeout = DEFAULT_VALUE;
	LPTSTR pszProxyHost = NULL, pszProxyUser = NULL, pszProxyPass = NULL;
	LPTSTR pszReferer = NULL;
	ULONG iHttpInternetFlags = DEFAULT_VALUE, iHttpSecurityFlags = DEFAULT_VALUE;
	PQUEUE_ITEM pItem = NULL;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	// Request unload notification
	extra->RegisterPluginCallback( g_hInst, NsisMessageCallback );

	TRACE( _T("NSxfer!Transfer\n"));

	psz = (LPTSTR)MyAlloc( string_size * sizeof(TCHAR) );
	for (;;)
	{
		if (popstring( psz ) != 0)
			break;
		if (lstrcmpi( psz, _T( "/END" ) ) == 0)
			break;

		if (lstrcmpi( psz, _T( "/PRIORITY" ) ) == 0) {
			iPriority = popint();
		} else if (lstrcmpi( psz, _T( "/METHOD" ) ) == 0) {
			if (popstring( psz ) == 0) {
				assert(
					lstrcmpi( psz, _T( "GET" ) ) == 0 ||
					lstrcmpi( psz, _T( "POST" ) ) == 0 ||
					lstrcmpi( psz, _T( "HEAD" ) ) == 0
					);
				if (*psz) {
					MyFree( pszMethod );
					MyStrDup( pszMethod, psz );
				} else {
					/// TODO:
				}
			}
		} else if (lstrcmpi( psz, _T( "/URL" ) ) == 0) {
			if (popstring( psz ) == 0) {
				MyFree( pszUrl );
				MyStrDup( pszUrl, psz );
			}
		} else if (lstrcmpi( psz, _T( "/LOCAL" ) ) == 0) {
			if (popstring( psz ) == 0) {
				MyFree( pszFile );
				if (lstrcmpi( psz, TEXT_LOCAL_NONE ) == 0) {
					iLocalType = ITEM_LOCAL_NONE;
				} else if (lstrcmpi( psz, TEXT_LOCAL_MEMORY ) == 0) {
					iLocalType = ITEM_LOCAL_MEMORY;
				} else {
					iLocalType = ITEM_LOCAL_FILE;
					MyStrDup( pszFile, psz );
				}
			}
		} else if (lstrcmpi( psz, _T( "/HEADERS" ) ) == 0) {
			if (popstring( psz ) == 0) {
				MyFree( pszHeaders );
				MyStrDup( pszHeaders, psz );
			}
		} else if (lstrcmpi( psz, _T( "/DATA" ) ) == 0) {
			if (popstring( psz ) == 0) {
				MyFree( pData );
				iDataSize = 0;
#ifdef UNICODE
				pData = MyAlloc( string_size );
				if (pData) {
					WideCharToMultiByte( CP_ACP, 0, psz, -1, (LPSTR)pData, string_size, NULL, NULL );
					iDataSize = lstrlenA( (LPSTR)pData );	/// Don't trust what WideCharToMultiByte returns...
				}
#else
				MyStrDup( pData, psz );
				iDataSize = lstrlen( psz );
#endif
			}
		} else if (lstrcmpi( psz, _T( "/DATAFILE" ) ) == 0) {
			if (popstring( psz ) == 0) {
				HANDLE hFile = CreateFile( psz, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
				if (hFile != INVALID_HANDLE_VALUE) {
					ULONG iFileSize = GetFileSize( hFile, NULL );
					if (iFileSize != INVALID_FILE_SIZE || GetLastError() == ERROR_SUCCESS) {
						MyFree( pData );
						iDataSize = 0;
						pData = MyAlloc( iFileSize );
						if (pData) {
							if (!ReadFile( hFile, pData, iFileSize, &iDataSize, NULL )) {
								MyFree( pData );
								iDataSize = 0;
								assert( !"/DATAFILE: Failed to read" );
							}
						} else {
							assert( !"/DATAFILE: Failed to allocate memory" );
						}
					} else {
						assert( !"/DATAFILE: Failed to get size" );
					}
					CloseHandle( hFile );
				} else {
					assert( !"/DATAFILE: Failed to open" );
				}
			}
		} else if (lstrcmpi( psz, _T( "/TIMEOUTCONNECT" ) ) == 0) {
			iTimeoutConnect = popint();
		} else if (lstrcmpi( psz, _T( "/TIMEOUTRECONNECT" ) ) == 0) {
			iTimeoutReconnect = popint();
		} else if (lstrcmpi( psz, _T( "/OPTCONNECTRETRIES" ) ) == 0) {
			iOptConnectRetries = popint();
		} else if (lstrcmpi( psz, _T( "/OPTCONNECTTIMEOUT" ) ) == 0) {
			iOptConnectTimeout = popint();
		} else if (lstrcmpi( psz, _T( "/OPTRECEIVETIMEOUT" ) ) == 0) {
			iOptRecvTimeout = popint();
		} else if (lstrcmpi( psz, _T( "/PROXY" ) ) == 0) {
			if (popstring( psz ) == 0) {
				MyFree( pszProxyHost );
				MyStrDup( pszProxyHost, psz );
			}
		} else if (lstrcmpi( psz, _T( "/PROXYUSER" ) ) == 0) {
			if (popstring( psz ) == 0) {
				MyFree( pszProxyUser );
				MyStrDup( pszProxyUser, psz );
			}
		} else if (lstrcmpi( psz, _T( "/PROXYPASS" ) ) == 0) {
			if (popstring( psz ) == 0) {
				MyFree( pszProxyPass );
				MyStrDup( pszProxyPass, psz );
			}
		} else if (lstrcmpi( psz, _T( "/REFERER" ) ) == 0) {
			if (popstring( psz ) == 0) {
				MyFree( pszReferer );
				MyStrDup( pszReferer, psz );
			}
		} else if (lstrcmpi( psz, _T( "/INTERNETFLAGS" ) ) == 0) {
			ULONG iFlags = (ULONG)popint_or();
			if (iFlags != 0)
				iHttpInternetFlags = iFlags;
		} else if (lstrcmpi( psz, _T( "/SECURITYFLAGS" ) ) == 0) {
			ULONG iFlags = (ULONG)popint_or();
			if (iFlags != 0)
				iHttpSecurityFlags = iFlags;
		} else {
			TRACE( _T( "  [!] Unknown parameter \"%s\"\n" ), psz );
		}
	}

	// Add to the download queue
	QueueLock( &g_Queue );
	QueueAdd(
		&g_Queue,
		iPriority,
		pszUrl, iLocalType, pszFile,
		pszProxyHost, pszProxyUser, pszProxyPass,
		pszMethod, pszHeaders, pData, iDataSize,
		iTimeoutConnect, iTimeoutReconnect,
		iOptConnectRetries, iOptConnectTimeout, iOptRecvTimeout,
		pszReferer,
		iHttpInternetFlags, iHttpSecurityFlags,
		&pItem
		);
	pushint( pItem ? pItem->iId : 0 );	/// Return the item's ID
	QueueUnlock( &g_Queue );

	MyFree( psz );
	MyFree( pszMethod );
	MyFree( pszUrl );
	MyFree( pszFile );
	MyFree( pszHeaders );
	MyFree( pData );
	MyFree( pszProxyHost );
	MyFree( pszProxyUser );
	MyFree( pszProxyPass );
	MyFree( pszReferer );
}


//++ QueryGlobal
EXTERN_C __declspec(dllexport)
void __cdecl QueryGlobal(
	HWND   parent,
	int    string_size,
	TCHAR   *variables,
	stack_t **stacktop,
	extra_parameters *extra
	)
{
	LPTSTR psz;
	LPTSTR pParam[30];
	int iParamCount = 0, iDropCount = 0, i;

	ULONG iThreadCount;
	ULONG iCountTotal, iCountDone, iCountDownloading, iCountWaiting;
	ULONG iSpeed;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	TRACE( _T( "NSxfer!QueryGlobal\n" ) );

	/// Allocate the working buffer
	psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	assert( psz );

	// Lock the queue
	QueueLock( &g_Queue );

	/// Statistics
	QueueStatistics(
		&g_Queue,
		&iThreadCount,
		&iCountTotal, &iCountDone, &iCountDownloading, &iCountWaiting,
		&iSpeed
		);

	/// Pop all parameters and remember them
	while (TRUE) {
		if (popstring( psz ) != 0)
			break;
		if (lstrcmpi( psz, _T( "/END" ) ) == 0)
			break;
		if (iParamCount < ARRAYSIZE( pParam )) {
			MyStrDup( pParam[iParamCount], psz );
			iParamCount++;
		} else {
			/// too many parameters
			iDropCount++;
		}
	}

	/// Return empty strings for dropped parameters
	for (i = 0; i < iDropCount; i++)
		pushstring( _T( "" ) );

	/// Iterate all parameters (in reverse order) and return their values
	for (i = iParamCount - 1; i >= 0; i--) {
		if (lstrcmpi( pParam[i], _T( "/COUNTTOTAL" ) ) == 0) {
			pushint( iCountTotal );
		} else if (lstrcmpi( pParam[i], _T( "/COUNTWAITING" ) ) == 0) {
			pushint( iCountWaiting );
		} else if (lstrcmpi( pParam[i], _T( "/COUNTDOWNLOADING" ) ) == 0) {
			pushint( iCountDownloading );
		} else if (lstrcmpi( pParam[i], _T( "/COUNTCOMPLETED" ) ) == 0) {
			pushint( iCountDone );
		} else if (lstrcmpi( pParam[i], _T( "/SPEEDBYTES" ) ) == 0) {
			pushint( iSpeed );
		} else if (lstrcmpi( pParam[i], _T( "/SPEED" ) ) == 0) {
			TCHAR szSpeed[50];
#ifdef UNICODE
			StrFormatByteSizeW( (LONGLONG)iSpeed, szSpeed, ARRAYSIZE( szSpeed ) );
#else
			StrFormatByteSizeA( iSpeed, szSpeed, ARRAYSIZE( szSpeed ) );
#endif
			lstrcat( szSpeed, _T( "/s" ) );
			pushstring( szSpeed );
		} else if (lstrcmpi( pParam[i], _T( "/COUNTTHREADS" ) ) == 0) {
			pushint( iThreadCount );
		} else {
			/// Unknown parameter. Return an empty string
			pushstring( _T( "" ) );
		}
		MyFree( pParam[i] );
	}

	// Unlock the queue
	QueueUnlock( &g_Queue );

	MyFree( psz );
}


//++ Query
EXTERN_C __declspec(dllexport)
void __cdecl Query(
	HWND   parent,
	int    string_size,
	TCHAR   *variables,
	stack_t **stacktop,
	extra_parameters *extra
	)
{
	LPTSTR psz;
	PQUEUE_ITEM pItem = NULL;

	LPTSTR pParam[30];
	int iParamCount = 0, iDropCount = 0, i;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	TRACE( _T( "NSxfer!Query\n" ) );

	/// Allocate the working buffer
	psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	assert( psz );

	// Lock the queue
	QueueLock( &g_Queue );

	/// Look for the transfer ID
	i = popint();
	for (pItem = g_Queue.pHead; pItem; pItem = pItem->pNext)
		if (pItem->iId == (ULONG)i)
			break;

	/// Pop all parameters and remember them
	while (TRUE) {
		if (popstring( psz ) != 0)
			break;
		if (lstrcmpi( psz, _T( "/END" ) ) == 0)
			break;
		if (iParamCount < ARRAYSIZE( pParam )) {
			MyStrDup( pParam[iParamCount], psz );
			iParamCount++;
		} else {
			/// too many parameters
			iDropCount++;
		}
	}

	/// Return empty strings for dropped parameters
	for (i = 0; i < iDropCount; i++)
		pushstring( _T( "" ) );

	/// Iterate all parameters (in reverse order) and return their values
	for (i = iParamCount - 1; i >= 0; i--) {
		if (pItem) {
			if (lstrcmpi( pParam[i], _T( "/PRIORITY" ) ) == 0) {
				pushint( pItem->iPriority );
			} else if (lstrcmpi( pParam[i], _T( "/STATUS" ) ) == 0) {
				switch (pItem->iStatus) {
				case ITEM_STATUS_WAITING:
					pushstring( TEXT_STATUS_WAITING );
					break;
				case ITEM_STATUS_DOWNLOADING:
					pushstring( TEXT_STATUS_DOWNLOADING );
					break;
				case ITEM_STATUS_DONE:
					pushstring( TEXT_STATUS_COMPLETED );
					break;
				default:
					pushstring( _T( "" ) );
				}
			} else if (lstrcmpi( pParam[i], _T( "/WININETSTATUS" ) ) == 0) {
				pushint( pItem->iLastCallbackStatus );
			} else if (lstrcmpi( pParam[i], _T( "/METHOD" ) ) == 0) {
				pushstring( pItem->szMethod );
			} else if (lstrcmpi( pParam[i], _T( "/URL" ) ) == 0) {
				pushstring( pItem->pszURL );
			} else if (lstrcmpi( pParam[i], _T( "/IP" ) ) == 0) {
				pushstring( pItem->pszSrvIP );
			} else if (lstrcmpi( pParam[i], _T( "/PROXY" ) ) == 0) {
				pushstring( pItem->pszProxy );
			} else if (lstrcmpi( pParam[i], _T( "/LOCAL" ) ) == 0) {
				switch (pItem->iLocalType) {
				case ITEM_LOCAL_NONE:
					pushstring( TEXT_LOCAL_NONE );
					break;
				case ITEM_LOCAL_FILE:
					pushstring( pItem->Local.pszFile );
					break;
				case ITEM_LOCAL_MEMORY:
					pushstring( TEXT_LOCAL_MEMORY );
					break;
				default:
					pushstring( _T( "" ) );
				}
			} else if (lstrcmpi( pParam[i], _T( "/SENTHEADERS" ) ) == 0) {
				pushstring( pItem->pszHeaders );
			} else if (lstrcmpi( pParam[i], _T( "/RECVHEADERS" ) ) == 0) {
				pushstring( pItem->pszSrvHeaders );
			} else if (lstrcmpi( pParam[i], _T( "/FILESIZE" ) ) == 0) {
				if (pItem->iFileSize != INVALID_FILE_SIZE64) {
					TCHAR sz[30];
					wnsprintf( sz, ARRAYSIZE( sz ), _T( "%I64u" ), pItem->iFileSize );
					pushstring( sz );
				} else {
					pushstring( _T( "" ) );
				}
			} else if (lstrcmpi( pParam[i], _T( "/RECVSIZE" ) ) == 0) {
				TCHAR sz[30];
				wnsprintf( sz, ARRAYSIZE( sz ), _T( "%I64u" ), pItem->iRecvSize );
				pushstring( sz );
			} else if (lstrcmpi( pParam[i], _T( "/PERCENT" ) ) == 0) {
				pushint( ItemGetRecvPercent( pItem ) );
			} else if (lstrcmpi( pParam[i], _T( "/SPEEDBYTES" ) ) == 0) {
				pushint( pItem->Speed.iSpeed );
			} else if (lstrcmpi( pParam[i], _T( "/SPEED" ) ) == 0) {
				pushstring( pItem->Speed.szSpeed );
			} else if (lstrcmpi( pParam[i], _T( "/CONTENT" ) ) == 0) {
				ItemMemoryContentToString( pItem, psz, string_size );
				pushstring( psz );
			} else if (lstrcmpi( pParam[i], _T( "/TIMEWAITING" ) ) == 0) {
				switch (pItem->iStatus) {
				case ITEM_STATUS_WAITING:
				{
					FILETIME tmNow;
					GetLocalFileTime( &tmNow );
					pushint( MyTimeDiff( &tmNow, &pItem->tmEnqueue ) );
					break;
				}
				case ITEM_STATUS_DOWNLOADING:
				case ITEM_STATUS_DONE:
				{
					pushint( MyTimeDiff( &pItem->tmConnect, &pItem->tmEnqueue ) );
					break;
				}
				default:
					pushstring( _T( "" ) );
				}
			} else if (lstrcmpi( pParam[i], _T( "/TIMEDOWNLOADING" ) ) == 0) {
				switch (pItem->iStatus) {
				case ITEM_STATUS_WAITING:
					pushstring( _T( "" ) );		/// No downloading time
					break;
				case ITEM_STATUS_DOWNLOADING:
				{
					FILETIME tmNow;
					GetLocalFileTime( &tmNow );
					pushint( MyTimeDiff( &tmNow, &pItem->tmConnect ) );
					break;
				}
				case ITEM_STATUS_DONE:
				{
					pushint( MyTimeDiff( &pItem->tmDisconnect, &pItem->tmConnect ) );
					break;
				}
				default:
					pushstring( _T( "" ) );
				}
			} else if (lstrcmpi( pParam[i], _T( "/ERRORCODE" ) ) == 0) {
				pushint( pItem->iWin32Error == ERROR_SUCCESS ? pItem->iHttpStatus : pItem->iWin32Error );
			} else if (lstrcmpi( pParam[i], _T( "/ERRORTEXT" ) ) == 0) {
				pushstring( pItem->iWin32Error == ERROR_SUCCESS ? pItem->pszHttpStatus : pItem->pszWin32Error );
			} else {
				/// Unknown parameter. Return an empty string
				pushstring( _T( "" ) );
			}
		} else {
			/// Unknown transfer ID
			pushstring( _T( "" ) );
		}
		MyFree( pParam[i] );
	}

	// Unlock the queue
	QueueUnlock( &g_Queue );

	MyFree( psz );
}


//++ Enumerate
EXTERN_C __declspec(dllexport)
void __cdecl Enumerate(
	HWND   parent,
	int    string_size,
	TCHAR   *variables,
	stack_t **stacktop,
	extra_parameters *extra
	)
{
	LPTSTR psz;
	ITEM_STATUS iStatus = ANY_STATUS;
	ULONG iPrio = ANY_PRIORITY;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	TRACE( _T( "NSxfer!Enumerate\n" ) );

	// Decide what items to enumerate
	psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	while (TRUE) {
		if (popstring( psz ) != 0)
			break;
		if (lstrcmpi( psz, _T( "/END" ) ) == 0) {
			break;
		} else if (lstrcmpi( psz, _T( "/STATUS" ) ) == 0) {
			if (popstring( psz ) == 0) {
				if (lstrcmpi( psz, TEXT_STATUS_DOWNLOADING ) == 0) {
					iStatus = ITEM_STATUS_DOWNLOADING;
				} else if (lstrcmpi( psz, TEXT_STATUS_WAITING ) == 0) {
					iStatus = ITEM_STATUS_WAITING;
				} else if (lstrcmpi( psz, TEXT_STATUS_COMPLETED ) == 0) {
					iStatus = ITEM_STATUS_DONE;
				//} else if (lstrcmpi( psz, _T( "paused" ) ) == 0) {
				//	iStatus = ITEM_STATUS_PAUSED;
				}
			}
		} else if (lstrcmpi( psz, _T( "/PRIORITY" ) ) == 0) {
			iPrio = popint();
		}
	}
	MyFree( psz );

	// Enumerate
	if (TRUE) {
		PQUEUE_ITEM pItem;
		int iCount = 0;
		QueueLock( &g_Queue );
		for (pItem = g_Queue.pHead; pItem; pItem = pItem->pNext) {
			if (ItemMatched( pItem, ANY_TRANSFER_ID, iPrio, iStatus )) {
				pushint( pItem->iId );
				iCount++;
			}
		}
		pushint( iCount );
		QueueUnlock( &g_Queue );
	}
}


//++ Wait
EXTERN_C __declspec(dllexport)
void __cdecl Wait(
	HWND   parent,
	int    string_size,
	TCHAR   *variables,
	stack_t **stacktop,
	extra_parameters *extra
	)
{
	INT_PTR iRet = 0;
	LPTSTR psz;
	UINT iId = ANY_TRANSFER_ID;
	ULONG iPrio = ANY_PRIORITY;
	GUI_MODE iMode = GUI_MODE_POPUP;
	HWND hTitle = NULL, hStatus = NULL, hProgress = NULL;
	LPTSTR pszTitleText = NULL, pszTitleMultiText = NULL;
	LPTSTR pszStatusText = NULL, pszStatusMultiText = NULL;
	BOOLEAN bAbort = FALSE;
	LPTSTR pszAbortTitle = NULL, pszAbortMsg = NULL;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	TRACE( _T( "NSxfer!Wait\n" ) );

	/// Working buffer
	psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	assert( psz );

	/// Parameters
	for (;;)
	{
		if (popstring( psz ) != 0)
			break;
		if (lstrcmpi( psz, _T( "/END" ) ) == 0)
			break;

		if (lstrcmpi( psz, _T( "/ID" ) ) == 0) {
			iId = popint();
		} else if (lstrcmpi( psz, _T( "/PRIORITY" ) ) == 0) {
			iPrio = popint();
		} else if (lstrcmpi( psz, _T( "/MODE" ) ) == 0) {
			if (popstring( psz ) == 0) {
				if (lstrcmpi( psz, _T( "SILENT" ) ) == 0)
					iMode = GUI_MODE_SILENT;
				else if (lstrcmpi( psz, _T( "POPUP" ) ) == 0)
					iMode = GUI_MODE_POPUP;
				else if (lstrcmpi( psz, _T( "PAGE" ) ) == 0)
					iMode = GUI_MODE_PAGE;
				else {
					iMode = GUI_MODE_POPUP;		/// Default
					TRACE( _T( "  [!] Unknown GUI mode \"%s\"\n" ), psz );
				}
			}
		} else if (lstrcmpi( psz, _T( "/TITLEHWND" ) ) == 0) {
			hTitle = (HWND)popintptr();
		} else if (lstrcmpi( psz, _T( "/STATUSHWND" ) ) == 0) {
			hStatus = (HWND)popintptr();
		} else if (lstrcmpi( psz, _T( "/PROGRESSHWND" ) ) == 0) {
			hProgress = (HWND)popintptr();
		} else if (lstrcmpi( psz, _T( "/TITLETEXT" ) ) == 0) {
			if (popstring( psz ) == 0) {
				MyFree( pszTitleText );
				MyStrDup( pszTitleText, psz );
			}
			if (popstring( psz ) == 0) {
				MyFree( pszTitleMultiText );
				MyStrDup( pszTitleMultiText, psz );
			}
		} else if (lstrcmpi( psz, _T( "/STATUSTEXT" ) ) == 0) {
			if (popstring( psz ) == 0) {
				MyFree( pszStatusText );
				MyStrDup( pszStatusText, psz );
			}
			if (popstring( psz ) == 0) {
				MyFree( pszStatusMultiText );
				MyStrDup( pszStatusMultiText, psz );
			}
		} else if (lstrcmpi( psz, _T( "/ABORT" ) ) == 0) {
			bAbort = TRUE;
			if (popstring( psz ) == 0) {
				MyFree( pszAbortTitle );
				MyStrDup( pszAbortTitle, psz );
			}
			if (popstring( psz ) == 0) {
				MyFree( pszAbortMsg );
				MyStrDup( pszAbortMsg, psz );
			}
		} else {
			TRACE( _T( "  [!] Unknown parameter \"%s\"\n" ), psz );
		}
	}

	// Wait
	iRet = GuiWait(
		iId, iPrio, iMode,
		hTitle, hStatus, hProgress,
		pszTitleText, pszTitleMultiText,
		pszStatusText, pszStatusMultiText,
		bAbort, pszAbortTitle, pszAbortMsg
		);

	MyFree( pszTitleText );
	MyFree( pszTitleMultiText );
	MyFree( pszStatusText );
	MyFree( pszStatusMultiText );
	MyFree( pszAbortTitle );
	MyFree( pszAbortMsg );
	MyFree( psz );
	pushintptr( iRet );
}


//++ Abort
EXTERN_C __declspec(dllexport)
void __cdecl Abort(
	HWND   parent,
	int    string_size,
	TCHAR   *variables,
	stack_t **stacktop,
	extra_parameters *extra
	)
{
	INT_PTR iRet = 0;
	LPTSTR psz;
	UINT iId = ANY_TRANSFER_ID;
	ULONG iPrio = ANY_PRIORITY;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	TRACE( _T( "NSxfer!Abort\n" ) );

	/// Working buffer
	psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	assert( psz );

	/// Parameters
	for (;;) {
		if (popstring( psz ) != 0)
			break;
		if (lstrcmpi( psz, _T( "/END" ) ) == 0)
			break;

		if (lstrcmpi( psz, _T( "/ID" ) ) == 0) {
			iId = popint();
		} else if (lstrcmpi( psz, _T( "/PRIORITY" ) ) == 0) {
			iPrio = popint();
		} else {
			TRACE( _T( "  [!] Unknown parameter \"%s\"\n" ), psz );
		}
	}

	// Abort
	if (TRUE) {
		PQUEUE_ITEM pItem;
		QueueLock( &g_Queue );
		for (pItem = g_Queue.pHead; pItem; pItem = pItem->pNext) {
			if (ItemMatched( pItem, iId, iPrio, ANY_STATUS )) {
				if (QueueAbort( &g_Queue, pItem )) {
					iRet++;
				}
			}
		}
		QueueUnlock( &g_Queue );
	}

	MyFree( psz );
	pushintptr( iRet );
}


//++ Test
EXTERN_C __declspec(dllexport)
void __cdecl Test(
	HWND   parent,
	int    string_size,
	TCHAR   *variables,
	stack_t **stacktop,
	extra_parameters *extra
	)
{
	EXDLL_INIT();
	EXDLL_VALIDATE();

	TRACE( _T( "NSxfer!Test\n" ) );

	/*{
		ULONGLONG ull, ull2;
		double d;

		ull = 1234567890;
		d = MyUlonglongToDouble( ull );
		ull2 = MyDoubleToUlonglong( d );
		assert( ull == ull2 );
	}*/

	/*{
		ULONG i;
		FILETIME t1, t2;
		GetSystemTimeAsFileTime( &t1 );
		Sleep( 1234 );
		GetSystemTimeAsFileTime( &t2 );
		i = MyTimeDiff( &t2, &t1 );
		assert( i == 1234 );
	}*/
}


//++ _DllMainCRTStartup
EXTERN_C
BOOL WINAPI _DllMainCRTStartup(
	HMODULE hInst,
	UINT iReason,
	LPVOID lpReserved
	)
{
	if ( iReason == DLL_PROCESS_ATTACH ) {
		g_hInst = hInst;
		PluginInit();
	}
	else if ( iReason == DLL_PROCESS_DETACH ) {
		PluginUninit( TRUE );
	}
	return TRUE;
}
