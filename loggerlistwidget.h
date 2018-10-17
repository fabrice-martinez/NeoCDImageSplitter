#ifndef LOGGERLISTWIDGET_H
#define LOGGERLISTWIDGET_H

#include <QDateTime>
#include <QListWidget>

#include "logger.h"

class LoggerListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit LoggerListWidget(QWidget *parent = Q_NULLPTR);
    virtual ~LoggerListWidget() Q_DECL_OVERRIDE;

public slots:
    void handleMessage(const LoggerMessage &message);
};

#endif // LOGGERLISTWIDGET_H
