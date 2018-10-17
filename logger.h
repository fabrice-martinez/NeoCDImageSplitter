#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QDateTime>
#include <QMessageLogger>

class LoggerMessage
{
public:
    explicit LoggerMessage() :
        timestamp(QDateTime::currentDateTime()),
        type(),
        message(),
        file(),
        function(),
        category(),
        line(0)
    { }

    explicit LoggerMessage(QtMsgType _type, const QMessageLogContext& _context, const QString& _msg) :
        timestamp(QDateTime::currentDateTime()),
        type(_type),
        message(_msg),
        file(_context.file ? QString::fromLatin1(_context.file) : QString()),
        function(_context.function ? QString::fromLatin1(_context.function) : QString()),
        category(_context.category ? QString::fromLatin1(_context.category) : QString()),
        line(_context.line)
    {}

    QDateTime timestamp;
    QtMsgType type;
    QString message;
    QString file;
    QString function;
    QString category;
    int line;
};

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = Q_NULLPTR);
    virtual ~Logger() Q_DECL_OVERRIDE;

signals:
    void newMessage(const LoggerMessage& message);
};

extern Logger* logger();

#endif // LOGGER_H
