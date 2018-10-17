#include "loggerlistwidget.h"
#include "logger.h"

#include <QStyle>

LoggerListWidget::LoggerListWidget(QWidget *parent) : QListWidget(parent)
{
    setSelectionMode(QAbstractItemView::ContiguousSelection);
    connect(logger(), &Logger::newMessage, this, &LoggerListWidget::handleMessage, Qt::QueuedConnection);
}

LoggerListWidget::~LoggerListWidget()
{ }

void LoggerListWidget::handleMessage(const LoggerMessage &message)
{
    QIcon icon;

    switch(message.type)
    {
    case QtInfoMsg:
        icon = style()->standardIcon(QStyle::SP_MessageBoxInformation);
        break;

    case QtWarningMsg:
        icon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
        break;

    case QtFatalMsg:
    case QtCriticalMsg:
        icon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
        break;

    default:
        icon = style()->standardIcon(QStyle::SP_MessageBoxQuestion);
        break;
    }

    QString messageStr = QString("%1: %2").arg(message.timestamp.toString(Qt::SystemLocaleShortDate)).arg(message.message);

    QListWidgetItem* item = new QListWidgetItem(icon, messageStr);
    addItem(item);

    scrollToBottom();
}
