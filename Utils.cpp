#include "Utils.h"

HRESULT EntryIDFromSourceKey(LPMDB lpStore, SBinary const& folderSource, SBinary const& itemSource, SBinary& entryID) {

  LPEXCHANGEMANAGESTORE lpManageStore = NULL;
  HRESULT hr = lpStore->QueryInterface(IID_IExchangeManageStore,
                                       (LPVOID*)&lpManageStore);

  if(hr != hrSuccess) {
    return hr;
  }

  LPENTRYID       lpEntryId = NULL;
  ULONG           cbEntryId = 0;

  hr = lpManageStore->EntryIDFromSourceKey(folderSource.cb,
                                           folderSource.lpb,
                                           itemSource.cb,
                                           itemSource.lpb,
                                           &cbEntryId,
                                           &lpEntryId);

  entryID.cb = cbEntryId;
  entryID.lpb = (LPBYTE) lpEntryId;
  
  return hrSuccess;
}

HRESULT EntryIDFromSourceKey(LPMDB lpStore, QString const& folderSource, SBinary& entryID) {
  SBinary folderSourceKey;
  Hex2Bin(folderSource, folderSourceKey);

  SBinary itemSourceKey;
  itemSourceKey.cb = 0;
  itemSourceKey.lpb = NULL;

  return EntryIDFromSourceKey(lpStore, folderSourceKey, itemSourceKey, entryID);
}

HRESULT EntryIDFromSourceKey(LPMDB lpStore, QString const& folderSource, QString const& itemSource, SBinary& entryID) {
  SBinary folderSourceKey;
  Hex2Bin(folderSource, folderSourceKey);

  SBinary itemSourceKey;
  Hex2Bin(itemSource, itemSourceKey);

  return EntryIDFromSourceKey(lpStore, folderSourceKey, itemSourceKey, entryID);

}


void Hex2Bin(QString const& input, SBinary& output) {
  Util::hex2bin(input.toStdString().c_str(), 
		strlen(input.toStdString().c_str()),
		&output.cb, &output.lpb, NULL);

}

void Bin2Hex(SBinary const& input, QString& output) {
  char* tmp = NULL;
  Util::bin2hex(input.cb, input.lpb, &tmp, NULL);
  output = tmp;
  delete tmp;
}
