#include "SendItemJob.h"

SendItemJob::SendItemJob(const Akonadi::Item &item, Session* session) : item(item), session(session) {
  lpMessage = NULL;
  lpFolder = NULL;
}

SendItemJob::~SendItemJob() {
  if(lpMessage) {
    lpMessage->Release();
  }
  if(lpFolder) {
    lpFolder->Release();
  }
}

void SendItemJob::start() {
  HRESULT hr = session->init();
  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;    
  }

  LPMDB lpStore = session->getLpStore();  
  IMAPISession *lpSession = session->getLpSession();
  IAddrBook *lpAddrBook = session->getLpAddrBook();
  ECLogger* lpLogger = session->getLpLogger();
  delivery_options* dopt = &session->dopt;

  KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  QByteArray bodyText = msg->encodedContent(true);

  LPSPropValue lpPropVal = NULL;
  hr = HrGetOneProp(lpStore, PR_IPM_OUTBOX_ENTRYID, &lpPropVal);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  ULONG ulObjType;
  hr = lpSession->OpenEntry(lpPropVal->Value.bin.cb, 
                            (LPENTRYID) lpPropVal->Value.bin.lpb, NULL, 
                            MAPI_MODIFY, &ulObjType, 
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

  hr = IMToMAPI(lpSession, lpStore, lpAddrBook, lpMessage, 
                bodyText.constData(), *dopt);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  SPropValue propName;

  propName.ulPropTag = PR_DELETE_AFTER_SUBMIT;
  propName.Value.b = TRUE;
  lpMessage->SetProps(1, &propName, NULL);

  propName.ulPropTag = PR_MESSAGE_CLASS;
  propName.Value.LPSZ = const_cast<TCHAR *>(_T("IPM.Note"));
  lpMessage->SetProps(1, &propName, NULL);

  lpMessage->SubmitMessage(0);

  emitResult();
}
