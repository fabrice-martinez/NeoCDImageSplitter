#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QIcon>

#include "cdromtoc.h"

namespace Ui {
class Dialog;
}

class QProgressDialog;

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = Q_NULLPTR);
    virtual ~Dialog() Q_DECL_OVERRIDE;

signals:
    void startExportSplitImage(const QString& baseDirectory, const QString& baseName, CdromToc* toc);

protected:
    void connectSignals();

    void updateActions();

    void updateTocView();

    void loadToc();

    void exportSplitImage();

    Ui::Dialog *ui;

    QIcon m_audioIcon;
    QIcon m_dataIcon;
    QIcon m_cdIcon;

    QProgressDialog* m_progressDialog;

    bool m_tocIsValid;
    CdromToc m_toc;
};

#endif // DIALOG_H
