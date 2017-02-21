#include "Utils.h"

HRESULT RawEntryIDFromSourceKey(LPMDB lpStore, SBinary const& folderSource, SBinary const& itemSource, SBinary& entryID) {

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

void Hex2Bin(QString const& input, SBinary& output) {
  Util::hex2bin(input.toStdString().c_str(), 
		strlen(input.toStdString().c_str()),
		&output.cb, &output.lpb, NULL);

}

void Bin2Hex(SBinary const& input, QString& output) {
  output = bin2hex(input.cb, input.lpb).c_str();
}
