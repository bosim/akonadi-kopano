#include "ItemsRemovedJob.h"

ItemsRemovedJob::ItemsRemovedJob(Akonadi::Item::List const& items, Session* session) : items(items), session(session) {
  lpFolder = NULL;
}

ItemsRemovedJob::~ItemsRemovedJob() {
  if(lpFolder) {
    lpFolder->Release();
  }
}

void ItemsRemovedJob::start() {
  HRESULT hr = session->init();
  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;    
  }

  LPMDB lpStore = session->getLpStore();

  QMap<QString, Akonadi::Item::List> folders;

  for(int i=0; i < items.count(); i++) {
    QString collectionId = items[i].parentCollection().remoteId();

    if(folders.find(collectionId) == folders.end()) {
      folders[collectionId] = Akonadi::Item::List();
    } 

    folders[collectionId] << items[i];
  }
  
  QList<QString> keys = folders.keys();

  for(int i=0; i < keys.count(); i++) {
    QString key = keys[i];
    SBinaryArray ba = {0, NULL};

    int count = folders[key].count();

    ba.cValues = count;
    ba.lpbin = new SBinary[count];

    SBinary sEntryID;
    session->EntryIDFromSourceKey(key, sEntryID);

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

    for(int j=0; j < count; j++) {
      Akonadi::Item item = folders[key][j];

      QStringList splitArr = item.remoteId().split(":");
      QString itemSourceKey = splitArr[1];
          
      session->EntryIDFromSourceKey(key, itemSourceKey, sEntryID);

      ba.lpbin[j] = sEntryID;
    }

    lpFolder->DeleteMessages(&ba, 0, NULL, 0);

    lpFolder->Release();
    lpFolder = NULL;
  }

  emitResult();
}
