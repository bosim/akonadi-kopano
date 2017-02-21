#include "RetrieveItemsJob.h"
#include "Synchronizer.h"

RetrieveItemsJob::RetrieveItemsJob(Akonadi::Collection const& collection, Session* session) : fullSync(false), collection(collection), session(session) {

  lpFolder = NULL;
  lpIEEC = NULL;
  lpStream = NULL;
}

RetrieveItemsJob::~RetrieveItemsJob() {
  /*if(lpIEEC) {
    lpIEEC->Release();
    }*/
  if(lpStream) {
    lpStream->Release();
  }
  if(lpFolder) {
    lpFolder->Release();
  }
}

void RetrieveItemsJob::start() {
  HRESULT hr = session->init();
  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;    
  }

  LPMDB lpStore = session->getLpStore();

  if(collection.parentCollection() == Akonadi::Collection::root()) {
    emitResult();
    return;
  }

  qDebug() << "Collection " << collection.remoteId();
  qDebug() << "Mimetypes " << collection.contentMimeTypes();

  SBinary sEntryID;
  QString cacheEntryID;

  session->EntryIDFromSourceKey(collection.remoteId(), sEntryID);

  ULONG ulObjType;
  hr = lpStore->OpenEntry(sEntryID.cb, (LPENTRYID) sEntryID.lpb, 
                          &IID_IMAPIFolder, MAPI_BEST_ACCESS, 
                          &ulObjType, (LPUNKNOWN *)&lpFolder);

  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  ULONG tmp[2] = { 0, 0 };
  hr = CreateStreamOnHGlobal(GlobalAlloc(GPTR, sizeof(tmp)), 
                             true, &lpStream);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }
  
  hr = lpFolder->OpenProperty(PR_CONTENTS_SYNCHRONIZER, 
                             &IID_IExchangeExportChanges, 0, 0, 
                             (LPUNKNOWN *)&lpIEEC);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  QByteArray state;
  bool synced = session->loadCollectionsState(collection.remoteId(), state);
  if(!synced) {
    fullSync = true;
  }

  lpStream->Write(state.data(), state.size(), NULL);
  lpStream->Commit(0);

  Synchronizer synchronizer;
  hr = lpIEEC->Config(lpStream, SYNC_NORMAL | SYNC_READ_STATE, 
                      &synchronizer, NULL, NULL, NULL, 0);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  ULONG ulSteps = 0;
  ULONG ulProgress = 0;
  do {
    hr = lpIEEC->Synchronize(&ulSteps, &ulProgress);
  } while(hr == SYNC_W_PROGRESS);

  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  /* Changed */
  for(int i=0; i < synchronizer.messagesChanged.count(); i++) {
    QString strSourceKey = synchronizer.messagesChanged[i];

    Akonadi::Item item(KMime::Message::mimeType());
    item.setParentCollection(collection);
    item.setRemoteId(collection.remoteId() + ":" + strSourceKey);
    item.setRemoteRevision(QString::number(1));

    items << item;
  }

  /* Read flag */
  for(int i=0; i < synchronizer.readStateChanged.count(); i++) {
    QPair<QString, bool> readState = synchronizer.readStateChanged[i];

    Akonadi::Item item(KMime::Message::mimeType());
    item.setParentCollection(collection);
    item.setRemoteId(collection.remoteId() + ":" + readState.first);
    item.setRemoteRevision(QString::number(1));

    if(readState.second) {
      item.setFlag(Akonadi::MessageFlags::Seen);
    } else {
      item.clearFlag(Akonadi::MessageFlags::Seen);  
    }

    items << item;
  }

 
  /* Deleted */
  for(int i=0; i < synchronizer.messagesDeleted.count(); i++) {
    QString strSourceKey = synchronizer.messagesDeleted[i];

    Akonadi::Item item(KMime::Message::mimeType());
    item.setParentCollection(collection);
    item.setRemoteId(collection.remoteId() + ":" + strSourceKey);
    item.setRemoteRevision(QString::number(1));

    deletedItems << item;
  }
  
  lpIEEC->UpdateState(lpStream);
  lpStream->Commit(0);

  state.clear();
  state.resize(sizeof(tmp));

  ULONG ulRead;
  hr = lpStream->Read(state.data(), state.size(), &ulRead);
  session->saveCollectionsState(collection.remoteId(), state);
  
  emitResult();

}
