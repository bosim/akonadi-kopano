#include "RetrieveItemsJob.h"

RetrieveItemsJob::RetrieveItemsJob(Akonadi::Collection const& collection, Session* session) : collection(collection), session(session) {

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

  session->init();

  LPMDB lpStore = session->getLpStore();

  enum { EID, FLAGS, NUM_COLS };
  SizedSPropTagArray(NUM_COLS, spt) = { 
    NUM_COLS, {PR_ENTRYID, PR_MESSAGE_FLAGS} 
  };

  if(collection.remoteId() == "/") {
    emitResult();
    return;
  }

  kDebug() << "Collection " <<collection.remoteId();

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
    hr = lpTable->QueryRows(MAX_ROWS, 0, &lpRowSet);
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

      kDebug() << strEntryID;
      
      item.setParentCollection(collection);
      item.setRemoteId(strEntryID);
      item.setRemoteRevision(QString::number(1));

      if(lpRowSet->aRow[i].lpProps[FLAGS].Value.ul & MSGFLAG_READ) {
        kDebug() << "MSG is READ";
        item.setFlag(Akonadi::MessageFlags::Seen);
      }
      else {
        kDebug() << "MSG is NOT READ";
        item.clearFlag(Akonadi::MessageFlags::Seen);
      }

      items << item;
      
    }

    FreeProws(lpRowSet);
    lpRowSet = NULL;
  }

  emitResult();

}
