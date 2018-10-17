#include <QDateTime>

#include "logger.h"

Q_DECLARE_METATYPE(QtMsgType)
Q_DECLARE_METATYPE(LoggerMessage)

Q_GLOBAL_STATIC(Logger, g_logger)

Logger* logger()
{
    return g_logger;
}

static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    logger()->newMessage(LoggerMessage(type, context, msg));
}

Logger::Logger(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<QtMsgType>();
    qRegisterMetaType<LoggerMessage>();

    qInstallMessageHandler(messageHandler);
}

Logger::~Logger()
{ }
