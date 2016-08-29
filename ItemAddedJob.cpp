#include "ItemAddedJob.h"

ItemAddedJob::ItemAddedJob(Akonadi::Item const& item, const Akonadi::Collection &collection, Session* session) : item(item), collection(collection), session(session) {
  lpFolder = NULL;
  lpMessage = NULL;
}

ItemAddedJob::~ItemAddedJob() {
  if(lpFolder) {
    lpFolder->Release();
  }
  if(lpMessage) {
    lpMessage->Release();
  }
}

void ItemAddedJob::start() {
  session->init();

  IMAPISession* lpSession = session->getLpSession();;
  LPMDB lpStore = session->getLpStore();
  IAddrBook* lpAddrBook = session->getLpAddrBook();;
  ECLogger* lpLogger = session->getLpLogger();;
  delivery_options* dopt = &session->dopt;

  SBinary sEntryID;
  Util::hex2bin(collection.remoteId().toStdString().c_str(), 
		strlen(collection.remoteId().toStdString().c_str()),
		&sEntryID.cb, &sEntryID.lpb, NULL);

  ULONG ulObjType;
  HRESULT hr = lpStore->OpenEntry(sEntryID.cb, (LPENTRYID) sEntryID.lpb, 
                                  NULL, MAPI_MODIFY, &ulObjType, 
                                  (LPUNKNOWN *) &lpFolder);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult(); 
    return;
  }

  hr = lpFolder->CreateMessage(NULL, 0, &lpMessage);        
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult(); 
    return;
  }

  if(!item.hasPayload<KMime::Message::Ptr>()) {
    setError(-1);
    emitResult();
    return;
  }

  KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  QByteArray bodyText = msg->encodedContent(true);

  hr = IMToMAPI(lpSession, lpStore, lpAddrBook, lpMessage, 
                bodyText.constData(), *dopt, lpLogger);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult(); 
    return;
  }

  SPropValue propName;
  propName.ulPropTag = PR_MESSAGE_CLASS;
  propName.Value.LPSZ = const_cast<TCHAR *>(_T("IPM.Note"));
  lpMessage->SetProps(1, &propName, NULL);  

  if(item.hasFlag(Akonadi::MessageFlags::Seen)) {
    propName.ulPropTag = PR_MESSAGE_FLAGS;
    propName.Value.ul = MSGFLAG_READ;
    lpMessage->SetProps(1, &propName, NULL);  
  } 

  lpMessage->SaveChanges(0);

  LPSPropValue lpPropVal = NULL;
  hr = HrGetOneProp(lpMessage, PR_SOURCE_KEY, &lpPropVal);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult(); 
    return;
  }

  QString strEntryID;
  Bin2Hex(lpPropVal->Value.bin, strEntryID);

  item.setRemoteId(strEntryID);
  item.setRemoteRevision(QString::number(1));
  item.setParentCollection(collection);

  emitResult();
}
