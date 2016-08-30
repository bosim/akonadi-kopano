#include "ItemsMovedJob.h"

ItemsMovedJob::ItemsMovedJob(Akonadi::Item::List const& items, const Akonadi::Collection &sourceCollection, const Akonadi::Collection &destinationCollection, Session* session) : items(items), sourceCollection(sourceCollection), destinationCollection(destinationCollection), session(session) {
  lpSrcFolder = NULL;
  lpDstFolder = NULL;
}

ItemsMovedJob::~ItemsMovedJob() {
  if(lpSrcFolder) {
    lpSrcFolder->Release();
  }
  if(lpDstFolder) {
    lpDstFolder->Release();
  }
}

void ItemsMovedJob::start() {
  session->init();

  LPMDB lpStore = session->getLpStore();

  SBinary sEntryID;
  QString remoteIdSrc = sourceCollection.remoteId();

  HRESULT hr = EntryIDFromSourceKey(lpStore, remoteIdSrc, sEntryID);
  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  kDebug() << "Source " << remoteIdSrc;

  ULONG ulObjType;
  hr = lpStore->OpenEntry(sEntryID.cb, 
			  (LPENTRYID) sEntryID.lpb, 
			  &IID_IMAPIFolder, 0, &ulObjType, 
			  (LPUNKNOWN *)&lpSrcFolder);

  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  QString remoteIdDst = destinationCollection.remoteId();

  kDebug() << "Destination " << remoteIdDst;

  hr = EntryIDFromSourceKey(lpStore, remoteIdDst, sEntryID);
  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  hr = lpStore->OpenEntry( sEntryID.cb, 
			  (LPENTRYID) sEntryID.lpb, 
			  &IID_IMAPIFolder, 0, &ulObjType, 
			  (LPUNKNOWN *)&lpDstFolder);

  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  kDebug() << "Items count " << items.count();

  ENTRYLIST sEntryList;

  sEntryList.cValues = items.count();

  hr = MAPIAllocateBuffer(sizeof(SBinary) * items.count(), 
			  (LPVOID *) &sEntryList.lpbin);

  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  for(int i=0; i < items.count(); i++) {
    QStringList splitArr = items[i].remoteId().split(":");
    QString itemSourceKey = splitArr[1];

    kDebug() << "Item " << itemSourceKey;

    EntryIDFromSourceKey(lpStore, remoteIdSrc, itemSourceKey, sEntryID);
    
    sEntryList.lpbin[i].cb = sEntryID.cb; 
    sEntryList.lpbin[i].lpb = sEntryID.lpb;

    if(hr != hrSuccess) {
      setError((int) hr);
      emitResult();
      return;
    }
  }

  kDebug() << "Moving messages";

  hr = lpSrcFolder->CopyMessages(&sEntryList, NULL, lpDstFolder,
				 0, NULL, MESSAGE_MOVE);  
  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  kDebug() << "Emitting result";

  emitResult();
}
