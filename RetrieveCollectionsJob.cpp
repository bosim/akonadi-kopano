#include "RetrieveCollectionsJob.h"


RetrieveCollectionsJob::RetrieveCollectionsJob(QString name, Session* session) : name(name), session(session) {
  lpFolder = NULL;
  lpTable = NULL;
  lpSortCriteria = NULL;
  lpRowSet = NULL;
}

RetrieveCollectionsJob::~RetrieveCollectionsJob() {
  if(lpRowSet) {
    FreeProws(lpRowSet);
  }

  if(lpSortCriteria) {
    MAPIFreeBuffer(lpSortCriteria);
  }

  if(lpTable) {
    lpTable->Release();
  }

  if(lpFolder) {
    lpFolder->Release();
  }
}

void RetrieveCollectionsJob::start() {
  HRESULT hr = session->init();
  if(hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;    
  }

  IMAPISession *lpSession = session->getLpSession();  
  LPMDB lpStore = session->getLpStore();  

  kDebug() << "Fetch collections";

  LPSPropValue lpPropVal = NULL;
  hr = HrGetOneProp(lpStore, PR_IPM_SUBTREE_ENTRYID, &lpPropVal);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  QStringList contentTypes;
  contentTypes << KMime::Message::mimeType() 
               << Akonadi::Collection::mimeType();

  Akonadi::Collection root;

  root.setName(name);
  root.setRemoteId("/");
  root.setContentMimeTypes(contentTypes);
  root.setParentCollection(Akonadi::Collection::root());

  collections.append(root);

  SBinary sEntryID = lpPropVal->Value.bin;
  ULONG ulObjType;
  hr = lpSession->OpenEntry(sEntryID.cb, (LPENTRYID) sEntryID.lpb, 
                            &IID_IMAPIFolder, 0, &ulObjType, 
                            (LPUNKNOWN *) &lpFolder);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  hr = lpFolder->GetHierarchyTable(CONVENIENT_DEPTH, &lpTable);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  enum { SKEY, PARENT_SKEY, NAME, CONTAINERCLASS, NUM_COLS };

  SizedSPropTagArray(NUM_COLS, spt) = { 
    NUM_COLS, {PR_SOURCE_KEY, PR_PARENT_SOURCE_KEY, PR_DISPLAY_NAME_W, 
               PR_CONTAINER_CLASS_A} 
  };

  hr = lpTable->SetColumns((LPSPropTagArray) &spt, 0);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  // Set sort criteria
  hr = MAPIAllocateBuffer(CbNewSSortOrderSet(1), (void**)&lpSortCriteria);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  lpSortCriteria->cCategories = 0;
  lpSortCriteria->cExpanded = 0;
  lpSortCriteria->cSorts = 1;
  lpSortCriteria->aSort[0].ulOrder = TABLE_SORT_ASCEND;
  lpSortCriteria->aSort[0].ulPropTag = PR_DEPTH;

  // Orders the rows of the table based on sort criteria
  hr = lpTable->SortTable(lpSortCriteria, 0);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  hr = lpTable->QueryRows(50, 0, &lpRowSet);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  if(lpRowSet->cRows == 0) {
    emitResult();
    return;
  }
	
  for (ULONG i = 0; i < lpRowSet->cRows; ++i) {
    LPSPropValue lpProps = lpRowSet->aRow[i].lpProps;

    if (PROP_TYPE(lpProps[CONTAINERCLASS].ulPropTag) == PT_STRING8 &&
      stricmp(lpProps[CONTAINERCLASS].Value.lpszA, "IPM") != 0 && 
      stricmp(lpProps[CONTAINERCLASS].Value.lpszA, "IPF.NOTE") != 0
    ) {
	continue;
    }


    if (PROP_TYPE(lpProps[NAME].ulPropTag) == PT_UNICODE) {
      char folderName[255];

      wcstombs(folderName, lpProps[NAME].Value.lpszW, 255);

      QString strSourceKey;
      Bin2Hex(lpProps[SKEY].Value.bin, strSourceKey);

      Akonadi::Collection collection;
      collection.setName(folderName);
      collection.setRemoteId(strSourceKey);
      collection.setContentMimeTypes(contentTypes);
      collection.setParentCollection(root);

      QString strParentSourceKey;
      Bin2Hex(lpProps[PARENT_SKEY].Value.bin, strParentSourceKey);
      
      for(int j=0; j < collections.count(); j++) {
        if(collections[j].remoteId() == strParentSourceKey) {
          collection.setParentCollection(collections[j]);
          break;
        }
      }

      collections.append(collection);

      kDebug() << "Got " << folderName;
      kDebug() << "EntryID " << strSourceKey;
      kDebug() << "Parent EntryID " << strParentSourceKey;

    }
  }

  emitResult();
}


