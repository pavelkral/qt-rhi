#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#if defined UTILS
#define UTILS_COMMON_DLLSPEC Q_DECL_EXPORT
#else
#define UTILS_COMMON_DLLSPEC Q_DECL_IMPORT
#endif


#include <QObject>

class UTILS_COMMON_DLLSPEC StringUtils : public QObject
{
    Q_OBJECT
public:
    explicit StringUtils(QObject *parent = nullptr);
    StringUtils(const StringUtils &) = delete;
    StringUtils(StringUtils &&) = delete;
    StringUtils &operator=(const StringUtils &) = delete;
    StringUtils &operator=(StringUtils &&) = delete;

    static QString mdToHtml(QString &S);
    static QString addHtmlStyle(const QString &str);

signals:
};

#endif // STRINGUTILS_H
