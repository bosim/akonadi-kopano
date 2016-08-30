#include "ItemsFlagsChangedJob.h"

ItemsFlagsChangedJob::ItemsFlagsChangedJob(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags, Session* session) : items(items), addedFlags(addedFlags), removedFlags(removedFlags), session(session) {
  lpMessage = NULL;

}

ItemsFlagsChangedJob::~ItemsFlagsChangedJob() {
  if(lpMessage) {
    lpMessage->Release();
  }
}

void ItemsFlagsChangedJob::start() {
  session->init();

  LPMDB lpStore = session->getLpStore();  

  for(int i=0; i < items.count(); i++) {
    Akonadi::Item item = items[i];

    QStringList splitArr = item.remoteId().split(":");
    QString collectionSourceKey = splitArr[0];
    QString itemSourceKey = splitArr[1];

    SBinary sEntryID;
    EntryIDFromSourceKey(lpStore, collectionSourceKey, 
                         itemSourceKey, sEntryID);

    ULONG ulObjType;
    HRESULT hr = lpStore->OpenEntry(sEntryID.cb, 
                                    (LPENTRYID) sEntryID.lpb, 
                                    &IID_IMessage, 
                                    MAPI_MODIFY, 
                                    &ulObjType, 
                                    (LPUNKNOWN *) &lpMessage);
		  
    if (hr != hrSuccess) {
      setError((int) hr);
      emitResult();
      return;
    }
    
    if(addedFlags.contains(Akonadi::MessageFlags::Seen)) {
      kDebug() << "Added MSGFLAG_READ";
      lpMessage->SetReadFlag(SUPPRESS_RECEIPT);
    } 
    else if(removedFlags.contains(Akonadi::MessageFlags::Seen)) {
      kDebug() << "Removed MSGFLAG_READ";
      lpMessage->SetReadFlag(CLEAR_READ_FLAG);
    }
    
  }

  emitResult();
}
