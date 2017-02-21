/*  akonadi-kopano
    Copyright (C) 2016 Bo Simonsen, bo@geekworld.dk

    Code based on akonadi airsync download by Martin Koller, kollix@aon.at

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RETRIEVECOLLECTIONSJOB_H
#define RETRIEVECOLLECTIONSJOB_H

#include <KJob>

#include <AkonadiAgentBase/ResourceBase>

#include <mapi.h>
#include <mapiutil.h>

#include <kopano/Util.h>
#include <KMime/Message>

#include <mapi.h>
#include <mapiutil.h>

#include "MapiSession.h"
#include "Utils.h"

class RetrieveCollectionsJob : public KJob {
  Q_OBJECT

 public:
    RetrieveCollectionsJob(QString name, Session* session); 
    ~RetrieveCollectionsJob();
    void start();
    
    Akonadi::Collection::List collections;
 private:
    Session* session;
    QString name;
    IMAPIFolder *lpFolder;	
    IMAPITable *lpTable;
    LPSSortOrderSet lpSortCriteria;
    LPSRowSet lpRowSet;

};

#endif
