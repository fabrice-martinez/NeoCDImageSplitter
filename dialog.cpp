#include "dialog.h"
#include "edc.h"
#include "qpropertymemory.h"
#include "ui_dialog.h"
#include "wavfile.h"

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTextStream>
#include <QtDebug>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    m_validTOCloaded(false)
{
    setWindowFlags(Qt::Window);

    ui->setupUi(this);

    m_audioIcon = QIcon(QStringLiteral(":/res/audio.png"));
    m_dataIcon = QIcon(QStringLiteral(":/res/data.png"));
    m_cdIcon = QIcon(QStringLiteral(":/res/cd.png"));

    updateDialog();
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::loadCUE()
{
    QString cueFilename = QFileDialog::getOpenFileName(this, tr("Open a CUE file"), "", tr("Cue Files (*.cue)"));
    if (cueFilename.isEmpty())
        return;

    parseCUE(cueFilename);
}

void Dialog::createSplitFileVersion()
{
    QString outputDirectory = QFileDialog::getExistingDirectory(this, tr("Choose the output directory"));
    if (outputDirectory.isEmpty())
        return;

    QString baseName = QInputDialog::getText(this, tr("Base Name"), tr("Enter the base name for the files"));
    if (baseName.isEmpty())
        return;

    outputSplitFiles(outputDirectory, baseName);
}

bool Dialog::parseCUE(const QString &cueFilename)
{
    static const QRegularExpression fileDirective(QStringLiteral("^\\s*FILE\\s+\"(.*)\"\\s+(\\S+)\\s*$"));
    static const QRegularExpression trackDirective(QStringLiteral("^\\s*TRACK\\s+([0-9]+)\\s+(\\S*)\\s*$"));
    static const QRegularExpression indexDirective(QStringLiteral("^\\s*INDEX\\s+([0-9]+)\\s+([0-9]+):([0-9]+):([0-9]+)\\s*$"));

    int trackNumber = 0;

    m_validTOCloaded = false;
    updateDialog();

    m_imageFilename.clear();
    m_toc.clear();

    QFile inFile(cueFilename);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        logMessage(tr("ERROR: Could not open input file %1:\n%2").arg(cueFilename).arg(inFile.errorString()));
        return false;
    }

    while(!inFile.atEnd())
    {
        QString line = inFile.readLine();

        QRegularExpressionMatch m = fileDirective.match(line);
        if (m.hasMatch())
        {
            if (m.captured(2) != QStringLiteral("BINARY"))
            {
                logMessage(tr("ERROR: FILE directive should have type BINARY."));
                return false;
            }

            if (!m_imageFilename.isEmpty())
            {
                logMessage(tr("ERROR: Only one FILE directive is supported by this tool."));
                return false;
            }

            m_imageFilename = m.captured(1);
            continue;
        }

        m = trackDirective.match(line);
        if (m.hasMatch())
        {
            trackNumber = m.captured(1).toInt();

            if ((trackNumber ==1) && (m.captured(2).compare(QStringLiteral("MODE1/2352"), Qt::CaseInsensitive)))
            {
                logMessage(tr("ERROR: Only MODE1/2352 is allowed for track 01."));
                return false;
            }
            else if ((trackNumber != 1) && (m.captured(2).compare(QStringLiteral("AUDIO"), Qt::CaseInsensitive)))
            {
                logMessage(tr("ERROR: Only AUDIO is allowed for track %1.").arg(trackNumber, 2, 10, QChar('0')));
                return false;
            }

            continue;
        }

        m = indexDirective.match(line);
        if (m.hasMatch())
        {
            if (!trackNumber)
            {
                logMessage(tr("ERROR: INDEX directive without TRACK directive."));
                return false;
            }

            int index = makeTocIndex(trackNumber, m.captured(1).toInt());
            m_toc[index] = msfToLba(m.captured(2).toInt(), m.captured(3).toInt(), m.captured(4).toInt());
        }
    }

    if (m_imageFilename.isEmpty())
    {
        logMessage(tr("ERROR: No FILE directive found."));
        return false;
    }

    QFileInfo info(cueFilename);

    m_imageFilename = QStringLiteral("%1/%2").arg(info.path()).arg(m_imageFilename);

    inFile.close();

    inFile.setFileName(m_imageFilename);
    if (!inFile.open(QIODevice::ReadOnly))
    {
        logMessage(tr("ERROR: Could not open image file %1:\n%2").arg(m_imageFilename).arg(inFile.errorString()));
        return false;
    }

    int64_t imageSize = inFile.size();
    if (imageSize % CDROM_SECTOR_SIZE)
    {
        logMessage(tr("ERROR: Image file size is not a multiple of %1.").arg(CDROM_SECTOR_SIZE));
        return false;
    }

    m_leadout = uint32_t(imageSize / CDROM_SECTOR_SIZE);

    logMessage(tr("INFO: CUE file loaded successfully."));
    m_validTOCloaded = true;
    updateDialog();
    return true;
}

bool Dialog::outputSplitFiles(const QString &baseOutPath, const QString &baseOutFilename)
{
    QPropertyMemory enabledMemory(this, QStringLiteral("enabled"));
    QPropertyMemory cursorMemory(this, QStringLiteral("cursor"));

    setEnabled(false);
    setCursor(Qt::WaitCursor);

    QFile inFile(m_imageFilename);
    if (!inFile.open(QIODevice::ReadOnly))
    {
        logMessage(tr("ERROR: Could not open input file %1:\n%2").arg(m_imageFilename).arg(inFile.errorString()));
        return false;
    }

    // Start by writing the ISO file

    if (!m_toc.contains(0x101))
    {
        logMessage(tr("ERROR: No track 1 index 1 in the toc."));
        return false;
    }

    QString outFilename;
    uint32_t startLBA;
    uint32_t endLBA;

    if (!getTrackBounds(1, startLBA, endLBA))
    {
        logMessage(tr("ERROR: Can't get bounds for track 01."));
        return false;
    }

    outFilename = buildSplitPath(1, baseOutPath, baseOutFilename);
    logMessage(tr("INFO: Writing %1.").arg(QFileInfo(outFilename).fileName()));

    QApplication::processEvents();

    inFile.seek(startLBA * CDROM_SECTOR_SIZE);

    if (!writeIsoFile(inFile, outFilename, endLBA - startLBA))
        return false;

    // Next, write all the audio tracks

    int track = 2;

    while(isTrackDefined(track))
    {
        if (!getTrackBounds(track, startLBA, endLBA))
        {
            logMessage(tr("ERROR: Can't get bounds for track %1.").arg(track, 2, 10, QChar('0')));
            return false;
        }

        inFile.seek(startLBA * CDROM_SECTOR_SIZE);

        outFilename = buildSplitPath(track, baseOutPath, baseOutFilename);
        logMessage(tr("INFO: Writing %1.").arg(QFileInfo(outFilename).fileName()));

        QApplication::processEvents();

        if (!writeWavFile(inFile, outFilename, (endLBA - startLBA) * CDROM_SECTOR_SIZE, 1, 2, 44100, 16))
            return false;

        ++track;
    }

    // Write a cue sheet file for our split version
    logMessage(tr("INFO: Writing cue sheet file."));

    QApplication::processEvents();

    if (!writeSplitCue(QStringLiteral("%1/%2.cue").arg(baseOutPath).arg(baseOutFilename), baseOutFilename, QStringLiteral("wav")))
        return false;

    // Write a ncd file for our split version
    logMessage(tr("INFO: Writing NCD file."));

    QApplication::processEvents();

    logMessage(tr("INFO: Completed successfully."));
    return true;
}

bool Dialog::writeIsoFile(QFile &inFile, const QString &outFilename, uint32_t sectorCount)
{
    QFile outFile(outFilename);
    if (!outFile.open(QIODevice::WriteOnly))
    {
        logMessage(tr("ERROR: Could not open output file %1:\n%2").arg(outFilename).arg(inFile.errorString()));
        return false;
    }

    while(sectorCount)
    {
        QByteArray buffer = inFile.read(CDROM_SECTOR_SIZE);

        if (buffer.size() != CDROM_SECTOR_SIZE)
        {
            logMessage(tr("ERROR: Could not read input file %1:\n%2").arg(inFile.fileName()).arg(inFile.errorString()));
            return false;
        }

        if (!checkSector(reinterpret_cast<const uint8_t*>(buffer.constData())))
        {
            logMessage(tr("ERROR: Image file %1 contains uncorrected read errors!").arg(inFile.fileName()));
            return false;
        }

        outFile.write(buffer.data() + CDROM_HEADER_SIZE, CDROM_DATA_SIZE);
        --sectorCount;
    }

    return true;
}

bool Dialog::writeWavFile(QFile &inFile, const QString &outFilename, uint32_t dataSize, uint16_t audioFormat, uint16_t channelCount, uint32_t sampleRate, uint16_t bitsPerSample)
{
    QFile outFile(outFilename);
    if (!outFile.open(QIODevice::WriteOnly))
    {
        logMessage(tr("ERROR: Could not open output file %1:\n%2").arg(outFile.fileName()).arg(outFile.errorString()));
        return false;
    }

    wave_riff_header_t riffHeader;
    wave_fmt_header_t fmtHeader;
    wave_data_header_t dataHeader;

    riffHeader.magic = 0x46464952;
    riffHeader.fileSize = dataSize + sizeof(fmtHeader) + sizeof(dataHeader) + 4;
    riffHeader.formatId = 0x45564157;

    fmtHeader.magic = 0x20746d66;
    fmtHeader.blockSize = sizeof(fmtHeader) - 8;
    fmtHeader.audioFormat = audioFormat;
    fmtHeader.channelCount = channelCount;
    fmtHeader.sampleRate = sampleRate;
    fmtHeader.bitsPerSample = bitsPerSample;
    fmtHeader.bytesPerBlock = channelCount * (bitsPerSample / 8);
    fmtHeader.bytesPerSecond = sampleRate * fmtHeader.bytesPerBlock;

    dataHeader.magic = 0x61746164;
    dataHeader.dataSize = dataSize;

    outFile.write(reinterpret_cast<const char *>(&riffHeader), sizeof(riffHeader));
    outFile.write(reinterpret_cast<const char *>(&fmtHeader), sizeof(fmtHeader));
    outFile.write(reinterpret_cast<const char *>(&dataHeader), sizeof(dataHeader));

    while(dataSize)
    {
        qint64 readSize = qMin(dataSize, uint32_t(1048576));

        QByteArray buffer = inFile.read(readSize);

        if (buffer.size() != readSize)
        {
            logMessage(tr("ERROR: Could not read input file %1:\n%2").arg(inFile.fileName()).arg(inFile.errorString()));
            return false;
        }

        qint64 done = outFile.write(buffer);

        if (done != readSize)
        {
            logMessage(tr("ERROR: Could not write to output file %1:\n%2").arg(outFile.fileName()).arg(outFile.errorString()));
            return false;
        }

        dataSize -= readSize;
    }

    return true;
}

bool Dialog::writeSplitCue(const QString& outFilename, const QString& baseFilename, const QString& audioExtension)
{
    QFile outFile(outFilename);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        logMessage(tr("ERROR: Could not open output file %1\n%2").arg(outFile.fileName()).arg(outFile.errorString()));
        return false;
    }

    QTextStream out(&outFile);

    int track = 0;
    uint32_t startLBA = 0;

    for(auto i = m_toc.cbegin(); i != m_toc.cend(); ++i)
    {
        if (i.key() >> 8 != track)
        {
            track = i.key() >> 8;
            startLBA = m_toc.value(makeTocIndex(track, 1));

            out << QStringLiteral("FILE \"");
            if (track == 1)
                out << buildSplitFilename(track, baseFilename, QStringLiteral("iso")) << QStringLiteral("\" BINARY") << endl;
            else
                out << buildSplitFilename(track, baseFilename, audioExtension) << QStringLiteral("\" WAVE") << endl;

            out << QStringLiteral("  TRACK %1 ").arg(track, 2, 10, QChar('0'));
            out << (track == 1 ? QStringLiteral("MODE1/2048") : QStringLiteral("AUDIO")) << endl;
        }

        int index = i.key() & 0xFF;

        if (!index)
            out << QStringLiteral("    PREGAP %1").arg(buildMsfString(startLBA - i.value())) << endl;
        else
            out << QStringLiteral("    INDEX %1 %2").arg(index, 2, 10, QChar('0')).arg(buildMsfString(i.value() - startLBA)) << endl;
    }

    return true;
}

bool Dialog::isTrackDefined(int track)
{
    return m_toc.contains(makeTocIndex(track, 1));
}

bool Dialog::getTrackBounds(int track, uint32_t &startLBA, uint32_t &endLBA)
{
    if (!m_toc.contains(makeTocIndex(track, 1)))
        return false;

    startLBA = m_toc.value(makeTocIndex(track, 1));

    if (m_toc.contains(makeTocIndex(track + 1, 0)))
        endLBA = m_toc.value(makeTocIndex(track + 1, 0));
    else if (m_toc.contains(makeTocIndex(track + 1, 1)))
        endLBA = m_toc.value(makeTocIndex(track + 1, 1));
    else
        endLBA = m_leadout;

    return true;
}

void Dialog::logMessage(const QString &msg)
{
    ui->logListWidget->addItem(msg);
    ui->logListWidget->scrollToBottom();
}

void Dialog::updateDialog()
{
    ui->createSplitVersionButton->setEnabled(m_validTOCloaded);

    ui->tocTableWidget->clear();
    ui->tocTableWidget->setColumnCount(4);
    ui->tocTableWidget->setRowCount(m_toc.count() == 0 ? 0 : m_toc.count() + 1);

    QTableWidgetItem* item = new QTableWidgetItem(tr("Track"));
    ui->tocTableWidget->setHorizontalHeaderItem(0, item);
    item = new QTableWidgetItem(tr("Index"));
    ui->tocTableWidget->setHorizontalHeaderItem(1, item);
    item = new QTableWidgetItem(tr("LBA"));
    ui->tocTableWidget->setHorizontalHeaderItem(2, item);
    item = new QTableWidgetItem(tr("MSF"));
    ui->tocTableWidget->setHorizontalHeaderItem(3, item);

    int j = 0;

    for(auto i = m_toc.constBegin(); i != m_toc.constEnd(); ++i)
    {
        uint32_t tocIndex = static_cast<uint32_t>(i.key());
        uint32_t lba = i.value();

        item = new QTableWidgetItem((tocIndex & 0xFF00) == 0x100 ? m_dataIcon : m_audioIcon, QString::number(tocIndex >> 8));
        ui->tocTableWidget->setItem(j, 0, item);
        item = new QTableWidgetItem(QString::number(tocIndex & 0xFF));
        ui->tocTableWidget->setItem(j, 1, item);
        item = new QTableWidgetItem(QString::number(lba));
        ui->tocTableWidget->setItem(j, 2, item);
        item = new QTableWidgetItem(buildMsfString(lba));
        ui->tocTableWidget->setItem(j, 3, item);

        ++j;
    }

    if (m_toc.count())
    {
        item = new QTableWidgetItem(m_cdIcon, tr("LEADOUT"));
        ui->tocTableWidget->setItem(j, 0, item);
        item = new QTableWidgetItem("");
        ui->tocTableWidget->setItem(j, 1, item);
        item = new QTableWidgetItem(QString::number(m_leadout));
        ui->tocTableWidget->setItem(j, 2, item);
        item = new QTableWidgetItem(buildMsfString(m_leadout));
        ui->tocTableWidget->setItem(j, 3, item);
    }

    ui->tocTableWidget->resizeColumnsToContents();
}

void Dialog::lbaToMsf(uint32_t lba, uint32_t &m, uint32_t &s, uint32_t &f)
{
    m = lba / (75 * 60);
    s = (lba / 75) % 60;
    f = lba % 75;
}

uint32_t Dialog::msfToLba(int m, int s, int f)
{
    return static_cast<uint32_t>((m * 60 * 75) + (s * 75) + f);
}

QString Dialog::buildMsfString(uint32_t lba)
{
    uint32_t m;
    uint32_t s;
    uint32_t f;

    lbaToMsf(lba, m, s, f);

    return QStringLiteral("%1:%2:%3")
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'))
            .arg(f, 2, 10, QChar('0'));
}

QString Dialog::buildSplitPath(int track, const QString &basePath, const QString &baseFilename)
{
    if (track == 1)
        return QStringLiteral("%1/%2").arg(basePath).arg(buildSplitFilename(track, baseFilename, QStringLiteral("iso")));

    return QStringLiteral("%1/%2").arg(basePath).arg(buildSplitFilename(track, baseFilename, QStringLiteral("wav")));
}

QString Dialog::buildSplitFilename(int track, const QString &baseFilename, const QString& extension)
{
    return QStringLiteral("%1-%2.%3").arg(track, 2, 10, QChar('0')).arg(baseFilename).arg(extension);
}
