#include "RetrieveCollectionsJob.h"


RetrieveCollectionsSubJob::RetrieveCollectionsSubJob(SBinary& sEntryID, Akonadi::Collection& parent, Session* session) : sEntryID(sEntryID), parent(parent), collections(), session(session) {
  lpFolder = NULL;
  lpTable = NULL;
  lpRowSet = NULL;
}

RetrieveCollectionsSubJob::~RetrieveCollectionsSubJob() {
  if (lpRowSet)
    FreeProws(lpRowSet);
  
  if (lpTable)
    lpTable->Release();

  if (lpFolder)
    lpFolder->Release();
}

void RetrieveCollectionsSubJob::start() {
  IMAPISession *lpSession = session->getLpSession();  

  enum { EID, NAME, SUBFOLDERS, CONTAINERCLASS, PARENT_EID, NUM_COLS };
  SizedSPropTagArray(NUM_COLS, spt) = { NUM_COLS, {PR_ENTRYID, PR_DISPLAY_NAME_W, PR_SUBFOLDERS, PR_CONTAINER_CLASS_A, PR_PARENT_ENTRYID } };
  char* strEntryID, *strParentEntryID;
  char folderName[255];

  QStringList contentTypes;
  contentTypes << KMime::Message::mimeType() 
	       << Akonadi::Collection::mimeType();

  ULONG ulObjType;
  HRESULT hr = lpSession->OpenEntry(sEntryID.cb, (LPENTRYID) sEntryID.lpb, 
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

  hr = lpTable->SetColumns((LPSPropTagArray) &spt, 0);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  // Set sort criteria
  LPSSortOrderSet lpSortCriteria = NULL;
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
    Akonadi::Collection collection;
    LPSPropValue lpProps = lpRowSet->aRow[i].lpProps;

    if (PROP_TYPE(lpProps[CONTAINERCLASS].ulPropTag) == PT_STRING8 &&
      stricmp(lpProps[CONTAINERCLASS].Value.lpszA, "IPM") != 0 && 
      stricmp(lpProps[CONTAINERCLASS].Value.lpszA, "IPF.NOTE") != 0
    ) {
	continue;
    }


    if (PROP_TYPE(lpProps[NAME].ulPropTag) == PT_UNICODE) {
      wcstombs(folderName, lpProps[NAME].Value.lpszW, 255);
      Util::bin2hex(lpProps[EID].Value.bin.cb, 
		    lpProps[EID].Value.bin.lpb, 
		    &strEntryID, NULL);

      kDebug() << "Got " << folderName;
      kDebug() << "EntryID " << strEntryID;

      collection.setName(folderName);
      collection.setRemoteId(strEntryID);
      collection.setContentMimeTypes(contentTypes);
      collection.setParentCollection(parent);

      Util::bin2hex(lpProps[PARENT_EID].Value.bin.cb, 
                    lpProps[PARENT_EID].Value.bin.lpb, 
                    &strParentEntryID, NULL);  
      
      for(int j=0; j < collections.count(); j++) {
        if(collections[j].remoteId() == strParentEntryID) {
          collection.setParentCollection(collections[j]);
          break;
        }
      }

      collections.append(collection);
    }
  }

  emitResult();
}

void RetrieveCollectionsSubJob::jobResult(KJob* job) {
  RetrieveCollectionsSubJob* req = qobject_cast<RetrieveCollectionsSubJob*>(job);
  
  if(req->error()) {
    setError((int) req->error());
  }
  
  for(int i=0; i < req->collections.size(); i++) {
    collections.append(req->collections[i]);
  }
}

/* Retrieve collections job */

RetrieveCollectionsJob::RetrieveCollectionsJob(QString name, Session* session) : name(name), session(session) {
  
}

RetrieveCollectionsJob::~RetrieveCollectionsJob() {
}

void RetrieveCollectionsJob::start() {
  session->init();

  LPMDB lpStore = session->getLpStore();  

  QStringList contentTypes;
  contentTypes << KMime::Message::mimeType() 
               << Akonadi::Collection::mimeType();


  kDebug() << "Fetch collections";

  LPSPropValue lpPropVal = NULL;
  HRESULT hr = HrGetOneProp(lpStore, PR_IPM_SUBTREE_ENTRYID, &lpPropVal);
  if (hr != hrSuccess) {
    setError((int) hr);
    emitResult();
    return;
  }

  SBinary sEntryID = lpPropVal->Value.bin;

  Akonadi::Collection root;
  root.setName(name);
  root.setRemoteId("/");
  root.setContentMimeTypes(contentTypes);
  root.setParentCollection(Akonadi::Collection::root());
  collections.append(root);

  RetrieveCollectionsSubJob* job = new RetrieveCollectionsSubJob(sEntryID, root, session);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(subJobResult(KJob*)));
  job->start();
}

void RetrieveCollectionsJob::subJobResult(KJob* job) {
  RetrieveCollectionsSubJob* req = qobject_cast<RetrieveCollectionsSubJob*>(job);

  if(req->error()) {
    setError(req->error());
    emitResult();
    return;
  }
  
  for(int i=0; i < req->collections.size(); i++) {
    collections.append(req->collections[i]);
  }  
  
  emitResult();
}
