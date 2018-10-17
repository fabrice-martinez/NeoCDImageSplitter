#include "dialog.h"
#include "imagewriterworker.h"
#include "ui_dialog.h"
#include "wavfile.h"

#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QProgressDialog>
#include <QtDebug>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    m_audioIcon(QStringLiteral(":/res/audio.png")),
    m_dataIcon(QStringLiteral(":/res/data.png")),
    m_cdIcon(QStringLiteral(":/res/cd.png")),
    m_progressDialog(new QProgressDialog(this)),
    m_tocIsValid(false),
    m_toc()
{
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setWindowFlags(m_progressDialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_progressDialog->reset();

    setWindowFlags(Qt::Window);

    ui->setupUi(this);

    connectSignals();
    updateActions();
    updateTocView();
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::connectSignals()
{
    connect(ui->loadCueButton, &QPushButton::clicked, this, &Dialog::loadToc);
    connect(ui->createSplitVersionButton, &QPushButton::clicked, this, &Dialog::exportSplitImage);
}

void Dialog::updateActions()
{
    ui->createSplitVersionButton->setEnabled(m_tocIsValid);
}

void Dialog::updateTocView()
{
    ui->tocTableWidget->clear();
    ui->tocTableWidget->setColumnCount(4);
    ui->tocTableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Track")));
    ui->tocTableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Index")));
    ui->tocTableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Start")));
    ui->tocTableWidget->setHorizontalHeaderItem(3, new QTableWidgetItem(tr("Length")));

    if (!m_tocIsValid)
        return;

    ui->tocTableWidget->setRowCount(m_toc.toc().size());

    int row = 0;

    for(const CdromToc::Entry& entry : m_toc.toc())
    {
        bool isData = (entry.trackType == CdromToc::TrackType::Mode1_2048) || (entry.trackType == CdromToc::TrackType::Mode1_2352);

        QString text = QStringLiteral("%1").arg(entry.trackIndex.track(), 2, 10, QChar('0'));
        ui->tocTableWidget->setItem(row, 0, new QTableWidgetItem(isData ? m_dataIcon : m_audioIcon, text));

        text = QStringLiteral("%1").arg(entry.trackIndex.index(), 2, 10, QChar('0'));
        ui->tocTableWidget->setItem(row, 1, new QTableWidgetItem(text));

        uint32_t m;
        uint32_t s;
        uint32_t f;
        CdromToc::toMSF(entry.startSector, m, s, f);

        text = QStringLiteral("%1:%2:%3").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')).arg(f, 2, 10, QChar('0'));
        ui->tocTableWidget->setItem(row, 2, new QTableWidgetItem(text));

        CdromToc::toMSF(entry.trackLength, m, s, f);
        text = QStringLiteral("%1:%2:%3").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')).arg(f, 2, 10, QChar('0'));
        ui->tocTableWidget->setItem(row, 3, new QTableWidgetItem(text));

        ++row;
    }

    ui->tocTableWidget->resizeColumnsToContents();
}

void Dialog::loadToc()
{
    QString cueFilename = QFileDialog::getOpenFileName(this, tr("Open a CUE file"), "", tr("Cue Files (*.cue)"));
    if (cueFilename.isEmpty())
        return;

    m_tocIsValid = m_toc.loadCueSheet(cueFilename);

    updateActions();

    updateTocView();

    if (m_tocIsValid)
        qInfo().noquote() << tr("TOC loaded successfully.");
}

void Dialog::exportSplitImage()
{
    QString outputDirectory = QFileDialog::getExistingDirectory(this, tr("Choose the output directory"));
    if (outputDirectory.isEmpty())
        return;

    QString baseName = QInputDialog::getText(this, tr("Base Name"), tr("Enter the base name for the files"));
    if (baseName.isEmpty())
        return;

    ImageWriterWorker* worker = new ImageWriterWorker;

    connect(this, &Dialog::startExportSplitImage, worker, &ImageWriterWorker::start);
    connect(worker, &ImageWriterWorker::finished, worker, &ImageWriterWorker::deleteLater);

    connect(worker, &ImageWriterWorker::started, m_progressDialog, &QProgressDialog::show);
    connect(worker, &ImageWriterWorker::finished, m_progressDialog, &QProgressDialog::reset);
    connect(worker, &ImageWriterWorker::progressRangeChanged, m_progressDialog, &QProgressDialog::setRange);
    connect(worker, &ImageWriterWorker::progressTextChanged, m_progressDialog, &QProgressDialog::setLabelText);
    connect(worker, &ImageWriterWorker::progressValueChanged, m_progressDialog, &QProgressDialog::setValue);

    emit startExportSplitImage(outputDirectory, baseName, &m_toc);
}

