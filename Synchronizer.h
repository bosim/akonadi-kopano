#include <edkguid.h>
#include <mapi.h>
#include <mapiutil.h>
#include <kopano/Util.h>
#include <kopano/ECLogger.h>

class Synchronizer  : public IExchangeImportContentsChanges {
 public:
  Synchronizer() {
    this->m_cRef = 1;
  }
  ULONG AddRef() {
    return ++m_cRef;
  };
  ULONG Release() {
    if (--m_cRef == 0) {
      delete this;
      return 0;
    } 
        
    return m_cRef;
  }
  HRESULT QueryInterface(REFIID iid, void **lpvoid) {
    if(iid == IID_IExchangeImportContentsChanges) {
      AddRef();
      *lpvoid = this;
      return hrSuccess;
    }

    return MAPI_E_INTERFACE_NOT_SUPPORTED;
  }

  HRESULT GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError) {
    return hrSuccess;
  }

  HRESULT Config(LPSTREAM lpStream, ULONG ulFlags) {
    return hrSuccess;
  }

  HRESULT UpdateState(LPSTREAM lpStream) {
    return hrSuccess;
  }  
      
  HRESULT ImportMessageChange(ULONG cValue, LPSPropValue lpPropArray, ULONG ulFlags, LPMESSAGE * lppMessage) {
    LPSPropValue lpPropValE = PpropFindProp(lpPropArray, cValue, PR_ENTRYID);
    char* strEntryID = NULL;
    Util::bin2hex(lpPropValE->Value.bin.cb, lpPropValE->Value.bin.lpb, 
                  &strEntryID, NULL);
    
    LPSPropValue lpPropValS = PpropFindProp(lpPropArray, cValue, PR_SOURCE_KEY);
    char* strSourceID = NULL;
    Util::bin2hex(lpPropValS->Value.bin.cb, lpPropValS->Value.bin.lpb, 
                  &strSourceID, NULL);

    messagesChanged.push_back(QPair<QString, QString>(QString(strEntryID), QString(strSourceID)));
       
    return hrSuccess;
  }
  HRESULT ImportMessageDeletion(ULONG ulFlags, LPENTRYLIST lpSourceEntryList) {
    char* strEntryID;
    for(unsigned int i=0; i < lpSourceEntryList->cValues; i++) {
      Util::bin2hex(lpSourceEntryList->lpbin[i].cb, 
                    lpSourceEntryList->lpbin[i].lpb, &strEntryID, NULL);

      messagesDeleted.push_back(QString(strEntryID));
    }
    return hrSuccess;
  }
  HRESULT ImportMessageMove(ULONG cbSourceKeySrcFolder, BYTE FAR * pbSourceKeySrcFolder, ULONG cbSourceKeySrcMessage, BYTE FAR * pbSourceKeySrcMessage, ULONG cbPCLMessage, BYTE FAR * pbPCLMessage, ULONG cbSourceKeyDestMessage, BYTE FAR * pbSourceKeyDestMessage, ULONG cbChangeNumDestMessage, BYTE FAR * pbChangeNumDestMessage) {
    return MAPI_E_NO_SUPPORT;
  }

  HRESULT ImportPerUserReadStateChange(ULONG cElements, LPREADSTATE lpReadState) {
    char* strEntryID;
    for(unsigned int i=0; i < cElements; i++) {
      Util::bin2hex(lpReadState[i].cbSourceKey, lpReadState[i].pbSourceKey,
                    &strEntryID, NULL);

      readStateChanged.push_back(QPair<QString, bool>(QString(strEntryID), lpReadState[i].ulFlags > 0));
    }
    return hrSuccess;
  }

  QList<QPair<QString, QString> > messagesChanged;
  QList<QString> messagesDeleted;
  QList<QPair<QString, bool> > readStateChanged;

 private:
  ULONG m_cRef;

};
