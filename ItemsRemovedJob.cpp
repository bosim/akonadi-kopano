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
  session->init();

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
    Util::hex2bin(key.toStdString().c_str(), 
                  strlen(key.toStdString().c_str()),
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

    for(int j=0; j < count; j++) {
      Akonadi::Item item = folders[key][j];
      std::string remoteId = item.remoteId().toStdString();
          
      hr = Util::hex2bin(remoteId.c_str(), 
                         strlen(remoteId.c_str()),
                         &sEntryID.cb, &sEntryID.lpb, NULL);

      ba.lpbin[j] = sEntryID;
    }

    lpFolder->DeleteMessages(&ba, 0, NULL, 0);

    lpFolder->Release();
    lpFolder = NULL;
  }

  emitResult();
}
