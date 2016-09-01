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
  HRESULT hr = session->init();
  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;    
  }

  LPMDB lpStore = session->getLpStore();

  SBinary sEntryID;
  QString remoteIdSrc = sourceCollection.remoteId();

  hr = session->EntryIDFromSourceKey(remoteIdSrc, sEntryID);
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

  hr = session->EntryIDFromSourceKey(remoteIdDst, sEntryID);
  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  hr = lpStore->OpenEntry( sEntryID.cb, 
			  (LPENTRYID) sEntryID.lpb, 
			  &IID_IMAPIFolder, MAPI_MODIFY, &ulObjType, 
			  (LPUNKNOWN *)&lpDstFolder);

  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  kDebug() << "Items count " << items.count();

  SBinaryArray ba = {0, NULL};
  ba.cValues = items.count();
  ba.lpbin = new SBinary[items.count()];

  for(int i=0; i < items.count(); i++) {
    QStringList splitArr = items[i].remoteId().split(":");
    QString itemSourceKey = splitArr[1];
    
    LPMESSAGE lpSrcMessage = NULL;
    LPMESSAGE lpDstMessage = NULL;

    kDebug() << "Item " << itemSourceKey;

    session->EntryIDFromSourceKey(remoteIdSrc, itemSourceKey, sEntryID);

    hr = lpStore->OpenEntry(sEntryID.cb, (LPENTRYID) sEntryID.lpb, 
                       &IID_IMessage, 0, &ulObjType, 
                       (LPUNKNOWN*)&lpSrcMessage);

    if(hr != hrSuccess) {
      kDebug() << "OpenEntry failed";
      setError((int) hr);
      emitResult();
      return;
    }

    hr = lpDstFolder->CreateMessage(NULL, 0, 
                                    &lpDstMessage);

    if(hr != hrSuccess) {
      kDebug() << "CreateMessage failed";
      setError((int) hr);
      emitResult();
      return;
    }

    hr = lpSrcMessage->CopyTo(0, NULL, NULL, 0, NULL, &IID_IMessage,
                              (LPVOID) lpDstMessage, 0, NULL);

    if(hr != hrSuccess) {
      kDebug() << "CopyTo failed";
      setError((int) hr);
      emitResult();
      return;
    }
    
    hr = lpDstMessage->SaveChanges(0);

    if(hr != hrSuccess) {
      kDebug() << "SaveChanges failed";
      setError((int) hr);
      emitResult();
      return;
    }

    ba.lpbin[i] = sEntryID;

    lpSrcMessage->Release();
    lpSrcMessage = NULL;
    
    LPSPropValue lpPropVal = NULL;
    hr = HrGetOneProp(lpDstMessage, PR_SOURCE_KEY, &lpPropVal);
    if (hr != hrSuccess) {
      kDebug() << "GetOneProp failed";
      setError((int) hr);
      emitResult(); 
      return;
    }

    QString strSourceKey;
    Bin2Hex(lpPropVal->Value.bin, strSourceKey);
    
    kDebug() << "New source key " << strSourceKey;

    items[i].setRemoteId(remoteIdDst + ":" + strSourceKey);
    items[i].setRemoteRevision(QString::number(1));
    items[i].setParentCollection(destinationCollection);

    lpDstMessage->Release();
    lpDstMessage = NULL;
  }

  lpSrcFolder->DeleteMessages(&ba, 0, NULL, 0);

  kDebug() << "Emitting result";

  emitResult();
}
