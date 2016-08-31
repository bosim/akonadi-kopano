#include "RetrieveItemsJob.h"
#include "Synchronizer.h"

RetrieveItemsJob::RetrieveItemsJob(Akonadi::Collection const& collection, Session* session) : fullSync(false), collection(collection), session(session) {

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

  if(collection.parentCollection() == Akonadi::Collection::root()) {
    emitResult();
    return;
  }

  kDebug() << "Collection " <<collection.remoteId();

  SBinary sEntryID;
  QString cacheEntryID;

  session->EntryIDFromSourceKey(collection.remoteId(), sEntryID);

  ULONG ulObjType;
  HRESULT hr = lpStore->OpenEntry(sEntryID.cb, 
				  (LPENTRYID) sEntryID.lpb, 
				  &IID_IMAPIFolder, MAPI_BEST_ACCESS, 
                                  &ulObjType, 
				  (LPUNKNOWN *)&lpFolder);

  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  LPSTREAM lpStream = NULL;
  ULONG tmp[2] = { 0, 0 };
  hr = CreateStreamOnHGlobal(GlobalAlloc(GPTR, sizeof(tmp)), 
                             true, &lpStream);
  
  IExchangeExportChanges *lpIEEC = NULL;
  hr = lpFolder->OpenProperty(PR_CONTENTS_SYNCHRONIZER, 
                             &IID_IExchangeExportChanges, 0, 0, 
                             (LPUNKNOWN *)&lpIEEC);

  if (hr != hrSuccess) {
  }


  bool synced = session->syncState.loadState(collection.remoteId(), tmp);
  if(!synced) {
    fullSync = true;
  }

  lpStream->Write(tmp, sizeof(tmp), NULL);
  lpStream->Commit(0);

  Synchronizer synchronizer;
  hr = lpIEEC->Config(lpStream, SYNC_NORMAL | SYNC_READ_STATE, 
                      &synchronizer, NULL, NULL, NULL, 0);
  if (hr != hrSuccess) {
  }

  ULONG ulSteps = 0;
  ULONG ulProgress = 0;
  do {
    hr = lpIEEC->Synchronize(&ulSteps, &ulProgress);
    if(hr != hrSuccess) {

    }
  } while(hr == SYNC_W_PROGRESS);

  /* Changed */
  for(int i=0; i < synchronizer.messagesChanged.count(); i++) {
    QString strSourceKey = synchronizer.messagesChanged[i];

    Akonadi::Item item;
    item.setParentCollection(collection);
    item.setRemoteId(collection.remoteId() + ":" + strSourceKey);
    item.setRemoteRevision(QString::number(1));

    items << item;
  }

  /* Deleted */
  for(int i=0; i < synchronizer.messagesDeleted.count(); i++) {
    QString strSourceKey = synchronizer.messagesDeleted[i];

    Akonadi::Item item;
    item.setParentCollection(collection);
    item.setRemoteId(collection.remoteId() + ":" + strSourceKey);
    item.setRemoteRevision(QString::number(1));

    deletedItems << item;
  }
  
  lpIEEC->UpdateState(lpStream);
  lpStream->Commit(0);

  ULONG ulRead;
  hr = lpStream->Read(&tmp, sizeof(tmp), &ulRead);
  session->syncState.saveState(collection.remoteId(), tmp);
  
  emitResult();

}
