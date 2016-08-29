#include <QDir>

#include <mapi.h>
#include <mapiutil.h>

class SynchronizerState {
 public:

  SynchronizerState() {
    cacheDir = QDir::homePath() + "/.cache/akonadi_kopano";
    QDir qdir(cacheDir);
    if(!qdir.exists()) {
      qdir.mkpath(cacheDir);
    }
  }

  bool loadState(QString strEntryID, ULONG* result) {
    QString fileName = cacheDir + "/state_" + strEntryID + ".dat";    
    QFile in(fileName);

    if(!in.open(QIODevice::ReadOnly)) {
      return false;
    }

    QDataStream stream(&in);
    stream.readRawData((char*) result, 8);

    in.close();

    return true;
  }
  
  bool saveState(QString strEntryID, ULONG result[2]) {
    QString fileName = cacheDir + "/state_" + strEntryID + ".dat";    
    QFile out(fileName);

    if(!out.open(QIODevice::WriteOnly)) {
      return false;
    }

    QDataStream stream(&out);
    stream.writeRawData((const char*) result, 8);

    out.close();

    return true;
    
  }
  
 private:
  QString cacheDir;
};
