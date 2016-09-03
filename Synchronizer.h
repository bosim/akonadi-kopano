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
    LPSPropValue lpPropValS = PpropFindProp(lpPropArray, cValue, 
                                            PR_SOURCE_KEY);
    QString sourceKey;
    Bin2Hex(lpPropValS->Value.bin, sourceKey);
    messagesChanged.push_back(sourceKey);
       
    return hrSuccess;
  }
  HRESULT ImportMessageDeletion(ULONG ulFlags, LPENTRYLIST lpSourceEntryList) {
    for(unsigned int i=0; i < lpSourceEntryList->cValues; i++) {
      QString sourceKey;
      Bin2Hex(lpSourceEntryList->lpbin[i], sourceKey);
      messagesDeleted.push_back(sourceKey);
    }
    return hrSuccess;
  }
  HRESULT ImportMessageMove(ULONG cbSourceKeySrcFolder, BYTE FAR * pbSourceKeySrcFolder, ULONG cbSourceKeySrcMessage, BYTE FAR * pbSourceKeySrcMessage, ULONG cbPCLMessage, BYTE FAR * pbPCLMessage, ULONG cbSourceKeyDestMessage, BYTE FAR * pbSourceKeyDestMessage, ULONG cbChangeNumDestMessage, BYTE FAR * pbChangeNumDestMessage) {
    return MAPI_E_NO_SUPPORT;
  }

  HRESULT ImportPerUserReadStateChange(ULONG cElements, LPREADSTATE lpReadState) {
    for(unsigned int i=0; i < cElements; i++) {
      SBinary sourceKey;
      QString strSourceKey;

      sourceKey.cb = lpReadState[i].cbSourceKey; 
      sourceKey.lpb = lpReadState[i].pbSourceKey;
      
      Bin2Hex(sourceKey, strSourceKey);

      readStateChanged.push_back(QPair<QString, bool>(strSourceKey, lpReadState[i].ulFlags > 0));
    }
    return hrSuccess;
  }

  QList<QString> messagesChanged;
  QList<QString> messagesDeleted;
  QList<QPair<QString, bool> > readStateChanged;

 private:
  ULONG m_cRef;

};
