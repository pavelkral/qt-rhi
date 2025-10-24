#ifndef LOGGER_H
#define LOGGER_H

#if defined UTILS
#define UTILS_COMMON_DLLSPEC Q_DECL_EXPORT
#else
#define UTILS_COMMON_DLLSPEC Q_DECL_IMPORT
#endif


#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDir>
#include <QApplication>

class UTILS_COMMON_DLLSPEC Logger {
public:
    static Logger& instance();
    void log(const QString& message, const QColor& col);
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setDebug(bool debug);
    bool isDebug() const;
    void reopenLogFile();

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void rotateLogFile();
    QString FilePath = QApplication::applicationDirPath() + QDir::separator() + "log.txt";
    QString OldFilePath = QApplication::applicationDirPath() + QDir::separator() + "log_old.txt";
    QFile logFile;
    QTextStream logStream;
    QMutex mutex;
    bool enabled = true;
    bool debug = false;
};

void UTILS_COMMON_DLLSPEC  myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

#define CUSTOM_WARNING(msg) \
myMessageHandler(QtWarningMsg, QMessageLogContext(__FILE__, __LINE__, Q_FUNC_INFO, nullptr), msg)

#endif // LOGGER_H

