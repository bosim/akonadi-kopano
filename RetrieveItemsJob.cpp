#include "RetrieveItemsJob.h"

RetrieveItemsJob::RetrieveItemsJob(Akonadi::Collection const& collection, Session* session) : collection(collection) {
  lpStore = session->getLpStore();

  lpFolder = NULL;
  lpTable = NULL;
  lpRowSet = NULL;
}

RetrieveItemsJob::~RetrieveItemsJob() {
  if(lpFolder) {
    lpFolder->Release();
  }
  if(lpTable) {
    lpTable->Release();
  }
  if(lpRowSet) {
    FreeProws(lpRowSet);    
  }
}

void RetrieveItemsJob::start() {

  enum { EID, SIZE, NUM_COLS };

  SizedSPropTagArray(NUM_COLS, spt) = { 
    NUM_COLS, {PR_ENTRYID, PR_MESSAGE_SIZE} 
  };

  if(collection.remoteId() == "/") {
    emitResult();
    return;
  }

  SBinary sEntryID;
  Util::hex2bin(collection.remoteId().toStdString().c_str(), 
		strlen(collection.remoteId().toStdString().c_str()),
		&sEntryID.cb, &sEntryID.lpb, NULL);

  ULONG ulObjType;
  HRESULT hr = lpStore->OpenEntry(sEntryID.cb, 
				  (LPENTRYID) sEntryID.lpb, 
				  &IID_IMAPIFolder, 0, &ulObjType, 
				  (LPUNKNOWN *)&lpFolder);

  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }
	
  hr = lpFolder->GetContentsTable(0, &lpTable);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  // Set the columns of the table
  hr = lpTable->SetColumns((LPSPropTagArray)&spt, 0);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  while(TRUE) {
    hr = lpTable->QueryRows(100, 0, &lpRowSet);
    if (hr != hrSuccess) {
      setError((int) hr);
      emitResult();
      return;
    }

    if (lpRowSet->cRows == 0)
      break;

    for (unsigned int i = 0; i < lpRowSet->cRows; i++) {

      Akonadi::Item item;
      char* strEntryID;

      Util::bin2hex(lpRowSet->aRow[i].lpProps[EID].Value.bin.cb, 
		    lpRowSet->aRow[i].lpProps[EID].Value.bin.lpb, 
		    &strEntryID, NULL);
      
      item.setParentCollection(collection);
      item.setRemoteId(strEntryID);
      item.setRemoteRevision(QString::number(1));

      items << item;
      
    }

    FreeProws(lpRowSet);
    lpRowSet = NULL;
  }

  emitResult();

}
