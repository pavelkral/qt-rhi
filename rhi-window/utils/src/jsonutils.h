#ifndef JSONUTILS_H
#define JSONUTILS_H

#if defined UTILS
#define UTILS_COMMON_DLLSPEC Q_DECL_EXPORT
#else
#define UTILS_COMMON_DLLSPEC Q_DECL_IMPORT

#endif
#include <QObject>
#include <QJsonDocument>
#include <QString>


class UTILS_COMMON_DLLSPEC JsonUtils : public QObject
{
    Q_OBJECT

public:

    JsonUtils() = delete;
    static QJsonDocument loadJsonDocumentFromFile(const QString &filePath);
    static bool saveJsonDocumentToFile(const QString &filePath,const QJsonDocument &jsonDocument,QJsonDocument::JsonFormat format = QJsonDocument::Indented);
 
private:


};

#endif // JSONUTILS_H


