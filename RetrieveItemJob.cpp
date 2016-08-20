#include "RetrieveItemJob.h"

RetrieveItemJob::RetrieveItemJob(Akonadi::Item const& item, Session* session) : item(item), session(session) {

  lpMessage = NULL;
}

RetrieveItemJob::~RetrieveItemJob() {
  if(lpMessage) {
    lpMessage->Release();
  }
}

void RetrieveItemJob::start() {
  session->init();

  LPMDB lpStore = session->getLpStore();
  IMAPISession* lpSession = session->getLpSession();
  IAddrBook* lpAddrBook = session->getLpAddrBook();
  ECLogger* lpLogger = session->getLpLogger();
  sending_options* sopt = &session->sopt;

  kDebug() << "retrieveItem";

  SBinary sEntryID;
  Util::hex2bin(item.remoteId().toStdString().c_str(), 
		strlen(item.remoteId().toStdString().c_str()),
		&sEntryID.cb, &sEntryID.lpb, NULL);

  ULONG ulObjType;
  HRESULT hr = lpStore->OpenEntry(sEntryID.cb, 
				  (LPENTRYID) sEntryID.lpb, 
				  &IID_IMessage, 
				  MAPI_DEFERRED_ERRORS, 
				  &ulObjType, 
				  (LPUNKNOWN *) &lpMessage);
		  
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
  }

  char *szMessage = NULL;
  hr = IMToINet(lpSession, lpAddrBook, lpMessage, &szMessage, 
		*sopt, lpLogger);

  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
  }

  KMime::Message::Ptr msg(new KMime::Message);
  QByteArray bodyText = szMessage;

  msg->setContent(KMime::CRLFtoLF(bodyText));
  msg->parse();

  item.setMimeType(QLatin1String("message/rfc822"));  // mail
  item.setPayload<KMime::Message::Ptr>(msg);

  emitResult();
}
