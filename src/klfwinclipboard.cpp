
#include <QDebug>
#include <QMimeData>

#include <windows.h>


void winClipboardCopy(HWND h, const char *type, const char * data, int size)
{
  if ( ! OpenClipboard(h) ) {
    qWarning("win: Cannot open the Clipboard");
    return;
  }
  // Remove the current Clipboard contents
  if( ! EmptyClipboard() ) {
    qWarning("win: Cannot empty the Clipboard");
    return;
  }
  // Register PNG datatype
  int wintype =
    RegisterClipboardFormat(reinterpret_cast<const WCHAR*>(QString::fromLatin1(type).utf16()));
  // Get the currently selected data
  HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, size);
  if ( ! hGlob ) {
    qWarning("win: Unable to GlobalAlloc(): %d", (int)GetLastError());
    return;
  }
  char * ptr = (char*)GlobalLock(hGlob);
  if (ptr == NULL) {
    qWarning("win: Unable to GlobalLock(): %d", (int)GetLastError());
    return;
  }
  memcpy(ptr, data, size);
  GlobalUnlock(hGlob);
  //  HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, 15);
  //  memcpy((char*)hGlob, "Hello World!\r\n", 15);
  // For the appropriate data formats...
  if ( ::SetClipboardData( wintype, hGlob ) == NULL ) {
    qWarning("win: Unable to set Clipboard data, error: %d", (int)GetLastError());
    CloseClipboard();
    GlobalFree(hGlob);
    return;
  }
  CloseClipboard();
}
