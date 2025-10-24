#include "logger.h"
#include <QDateTime>
#include <cstdio>
#include <QColor>
#include <QFileInfo>
#include <QDir>

Logger::Logger()
{


    logFile.setFileName(FilePath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
		logStream.setDevice(&logFile);
	}
    else {
        //qWarning() << "Failed to open log file:" << "Error:" << logFile.errorString();
        fprintf(stderr, "Failed to open log file: %s\n",
                logFile.errorString().toLocal8Bit().constData());
	}

}


Logger::~Logger() {
	if (logFile.isOpen()) {

        logStream.flush();
		logFile.close();
       // logStream.end
	}
}

Logger& Logger::instance() {
	static Logger loggerInstance;
	return loggerInstance;
}

void Logger::setEnabled(bool value) {
	QMutexLocker locker(&mutex);
	enabled = value;
}

bool Logger::isEnabled() const {
	return enabled;
}

void Logger::setDebug(bool debug) {
    this->debug = debug;
}

bool Logger::isDebug() const {
    return debug;
}

void Logger::reopenLogFile()
{
    if (logFile.isOpen()) {
        logFile.close();
    }

    logFile.setFileName(FilePath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to reopen log file";
        return;
    }

    logStream.setDevice(&logFile);
}

void Logger::log(const QString& message, const QColor& col) {

	QMutexLocker locker(&mutex);

	if (!enabled) return;

	//QString timeStampedMessage = QDateTime::currentDateTime()
		//.toString("yyyy-MM-dd HH:mm:ss.zzz") + " - " + message;

    QString timeStampedMessage = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") + " - " + message;
   // QString timeStampedMessage =  message;

	const qint64 maxSize = 1024 * 1024;

    if (!logFile.isOpen()) {
        reopenLogFile();
    }

	if (logFile.size() > maxSize) {
		rotateLogFile();
	}

	if (col == Qt::red) {
		fprintf(stdout, "\033[1;31m %s \033[0m\n", timeStampedMessage.toUtf8().constData());
	}
	else if (col == Qt::green) {
		//qDebug().noquote() << "\033[1;32m" << str << " \033[0m";
		fprintf(stdout, "\033[1;32m %s \033[0m\n", timeStampedMessage.toUtf8().constData());
	}
	else if (col == Qt::blue) {
		//qDebug().noquote() << "\033[1;34m" << message << " \033[0m";
		fprintf(stdout, "\033[1;34m %s \033[0m\n", timeStampedMessage.toUtf8().constData());
	}
	else if (col == Qt::yellow) {
		//qDebug().noquote() << "\033[1;33m" << str << " \033[0m";
		fprintf(stdout, "\033[1;33m %s \033[0m\n", timeStampedMessage.toUtf8().constData());
	}
	else if (col == Qt::cyan) {
		//qDebug().noquote() << "\033[1;36m" << str << " \033[0m";
		fprintf(stdout, "\033[1;36m %s \033[0m\n", timeStampedMessage.toUtf8().constData());
	}
	else if (col == Qt::magenta) {
		//qDebug().noquote() << "\033[1;35m" << str << " \033[0m";
		fprintf(stdout, "\033[1;35m %s \033[0m\n", timeStampedMessage.toUtf8().constData());
	}
	else {
		//qDebug().noquote() << "\033[1;37m" << str << " \033[0m";
		fprintf(stdout, "\033[1;37m %s \033[0m\n", timeStampedMessage.toUtf8().constData());
	}

	fflush(stdout);

	if (logFile.isOpen()) {
		logStream << timeStampedMessage << '\n';
		logStream.flush();
	}
}

void Logger::rotateLogFile() {

	if (logFile.isOpen()) {
		logStream.flush();
		logFile.close();
	}

    QFile::remove(OldFilePath);
    QFile::rename(FilePath, OldFilePath);

    logFile.setFileName(FilePath);
	if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		logStream.setDevice(&logFile);
	}
	else {
		fprintf(stderr, "Failed to open log file!\n");
	}
}

void myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	if (!Logger::instance().isEnabled())
		return;

	if (!Logger::instance().isDebug()) {
		if (type == QtDebugMsg || type == QtInfoMsg)
            return; // not warning/critical
	}


	QColor color(Qt::white);

	QString level;
	switch (type) {
        case QtDebugMsg:    level = "[DEBUG]"; color = Qt::green; break;
        case QtInfoMsg:     level = "[INFO]"; color = Qt::blue; break;
        case QtWarningMsg:  level = "[WARNING]"; color = Qt::yellow; break;
        case QtCriticalMsg: level = "[CRITICAL]"; color = Qt::red; break;
        case QtFatalMsg:    level = "[FATAL]"; color = Qt::red; break;
	}

	QString file = context.file ? context.file : "unknown";
	QString function = context.function ? context.function : "unknown";
    QString fileName = QFileInfo(file).fileName();

    // QString fullMessage = QString("%1 %2 (%3:%4), %5)")
    // 	.arg(level)
    // 	.arg(msg)
    //   .arg(file)
    // 	.arg(context.line)
    // 	.arg(function);

    QString fullMessage = QString("%1 %2 (%3:%4)")
                              .arg(level)
                              .arg(msg)
                              .arg(fileName)
                              .arg(context.line);


	Logger::instance().log(fullMessage, color);

	if (type == QtFatalMsg) {
		abort();
	}
}
