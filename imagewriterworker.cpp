#include "imagewriterworker.h"
#include "wavfile.h"
#include "wavstruct.h"

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QtDebug>
#include <array>

constexpr int CDROM_SECTOR_SIZE = 2352;
constexpr int CDROM_DATA_SIZE = 2048;
constexpr int CDROM_HEADER_SIZE = 16;

static const std::array<uint32_t, 256> EDCTABLE{
    0x00000000, 0x90910101, 0x91210201, 0x01b00300, 0x92410401, 0x02d00500, 0x03600600, 0x93f10701,
    0x94810801, 0x04100900, 0x05a00a00, 0x95310b01, 0x06c00c00, 0x96510d01, 0x97e10e01, 0x07700f00,
    0x99011001, 0x09901100, 0x08201200, 0x98b11301, 0x0b401400, 0x9bd11501, 0x9a611601, 0x0af01700,
    0x0d801800, 0x9d111901, 0x9ca11a01, 0x0c301b00, 0x9fc11c01, 0x0f501d00, 0x0ee01e00, 0x9e711f01,
    0x82012001, 0x12902100, 0x13202200, 0x83b12301, 0x10402400, 0x80d12501, 0x81612601, 0x11f02700,
    0x16802800, 0x86112901, 0x87a12a01, 0x17302b00, 0x84c12c01, 0x14502d00, 0x15e02e00, 0x85712f01,
    0x1b003000, 0x8b913101, 0x8a213201, 0x1ab03300, 0x89413401, 0x19d03500, 0x18603600, 0x88f13701,
    0x8f813801, 0x1f103900, 0x1ea03a00, 0x8e313b01, 0x1dc03c00, 0x8d513d01, 0x8ce13e01, 0x1c703f00,
    0xb4014001, 0x24904100, 0x25204200, 0xb5b14301, 0x26404400, 0xb6d14501, 0xb7614601, 0x27f04700,
    0x20804800, 0xb0114901, 0xb1a14a01, 0x21304b00, 0xb2c14c01, 0x22504d00, 0x23e04e00, 0xb3714f01,
    0x2d005000, 0xbd915101, 0xbc215201, 0x2cb05300, 0xbf415401, 0x2fd05500, 0x2e605600, 0xbef15701,
    0xb9815801, 0x29105900, 0x28a05a00, 0xb8315b01, 0x2bc05c00, 0xbb515d01, 0xbae15e01, 0x2a705f00,
    0x36006000, 0xa6916101, 0xa7216201, 0x37b06300, 0xa4416401, 0x34d06500, 0x35606600, 0xa5f16701,
    0xa2816801, 0x32106900, 0x33a06a00, 0xa3316b01, 0x30c06c00, 0xa0516d01, 0xa1e16e01, 0x31706f00,
    0xaf017001, 0x3f907100, 0x3e207200, 0xaeb17301, 0x3d407400, 0xadd17501, 0xac617601, 0x3cf07700,
    0x3b807800, 0xab117901, 0xaaa17a01, 0x3a307b00, 0xa9c17c01, 0x39507d00, 0x38e07e00, 0xa8717f01,
    0xd8018001, 0x48908100, 0x49208200, 0xd9b18301, 0x4a408400, 0xdad18501, 0xdb618601, 0x4bf08700,
    0x4c808800, 0xdc118901, 0xdda18a01, 0x4d308b00, 0xdec18c01, 0x4e508d00, 0x4fe08e00, 0xdf718f01,
    0x41009000, 0xd1919101, 0xd0219201, 0x40b09300, 0xd3419401, 0x43d09500, 0x42609600, 0xd2f19701,
    0xd5819801, 0x45109900, 0x44a09a00, 0xd4319b01, 0x47c09c00, 0xd7519d01, 0xd6e19e01, 0x46709f00,
    0x5a00a000, 0xca91a101, 0xcb21a201, 0x5bb0a300, 0xc841a401, 0x58d0a500, 0x5960a600, 0xc9f1a701,
    0xce81a801, 0x5e10a900, 0x5fa0aa00, 0xcf31ab01, 0x5cc0ac00, 0xcc51ad01, 0xcde1ae01, 0x5d70af00,
    0xc301b001, 0x5390b100, 0x5220b200, 0xc2b1b301, 0x5140b400, 0xc1d1b501, 0xc061b601, 0x50f0b700,
    0x5780b800, 0xc711b901, 0xc6a1ba01, 0x5630bb00, 0xc5c1bc01, 0x5550bd00, 0x54e0be00, 0xc471bf01,
    0x6c00c000, 0xfc91c101, 0xfd21c201, 0x6db0c300, 0xfe41c401, 0x6ed0c500, 0x6f60c600, 0xfff1c701,
    0xf881c801, 0x6810c900, 0x69a0ca00, 0xf931cb01, 0x6ac0cc00, 0xfa51cd01, 0xfbe1ce01, 0x6b70cf00,
    0xf501d001, 0x6590d100, 0x6420d200, 0xf4b1d301, 0x6740d400, 0xf7d1d501, 0xf661d601, 0x66f0d700,
    0x6180d800, 0xf111d901, 0xf0a1da01, 0x6030db00, 0xf3c1dc01, 0x6350dd00, 0x62e0de00, 0xf271df01,
    0xee01e001, 0x7e90e100, 0x7f20e200, 0xefb1e301, 0x7c40e400, 0xecd1e501, 0xed61e601, 0x7df0e700,
    0x7a80e800, 0xea11e901, 0xeba1ea01, 0x7b30eb00, 0xe8c1ec01, 0x7850ed00, 0x79e0ee00, 0xe971ef01,
    0x7700f000, 0xe791f101, 0xe621f201, 0x76b0f300, 0xe541f401, 0x75d0f500, 0x7460f600, 0xe4f1f701,
    0xe381f801, 0x7310f900, 0x72a0fa00, 0xe231fb01, 0x71c0fc00, 0xe151fd01, 0xe0e1fe01, 0x7070ff00
};

ImageWriterWorker::ImageWriterWorker(QObject *parent) :
    QObject(parent),
    m_cancelFlag(false),
    m_uncorrectedErrorsFlag(false)
{ }

ImageWriterWorker::~ImageWriterWorker()
{ }

void ImageWriterWorker::start(const QString &baseDirectory, const QString &baseName, CdromToc *toc)
{
    emit progressRangeChanged(0, static_cast<int>(toc->totalSectors()));
    emit progressValueChanged(0);
    emit progressTextChanged(QString());
    emit started();

    QCoreApplication::processEvents();

    if (!writeCueSheet(baseDirectory, baseName, toc))
    {
        emit finished();
        return;
    }

    uint8_t currentTrack = 0;
    int currentFile = -1;
    CdromToc::TrackType currentType = CdromToc::TrackType::Silence;
    uint32_t trackSectorsWritten = 0;
    uint32_t sectorsProcessed = 0;
    bool outFileIsWave = false;
    QFile in;
    QFile out;
    WavFile inWave;

    for(const CdromToc::Entry& entry : toc->toc())
    {
        if (m_cancelFlag)
            break;

        emit progressValueChanged(static_cast<int>(sectorsProcessed));
        QCoreApplication::processEvents();

        if (entry.trackIndex.track() != currentTrack)
        {
            if (out.isOpen())
            {
                if  (outFileIsWave)
                    writeWaveHeader(out, trackSectorsWritten * CDROM_SECTOR_SIZE);

                out.close();
            }

            m_uncorrectedErrorsFlag = false;

            currentTrack = entry.trackIndex.track();

            const CdromToc::Entry* firstEntry = toc->findTocEntry(TrackIndex{currentTrack, 1});
            if (!firstEntry)
            {
                qCritical().noquote() << "Internal error: Track " << currentTrack << " has no index 1!";
                break;
            }

            currentType = firstEntry->trackType;

            QString outSuffix;

            if ((currentType == CdromToc::TrackType::Mode1_2048) || (currentType == CdromToc::TrackType::Mode1_2352))
            {
                outSuffix = QStringLiteral("iso");
                outFileIsWave = false;
            }
            else
            {
                outSuffix = QStringLiteral("wav");
                outFileIsWave = true;
            }

            QString outFileName = buildTrackOutputFilename(entry.trackIndex, baseName, outSuffix);
            QString outFilePath = buildTrackOutputPath(baseDirectory, entry.trackIndex, baseName, outSuffix);

            emit progressTextChanged(tr("Writing: %1").arg(outFileName));

            out.setFileName(outFilePath);
            if (!out.open(QIODevice::WriteOnly))
            {
                qCritical().noquote() << "Could not create file: " << outFileName << endl << out.errorString() << endl;
                break;
            }

            if (outFileIsWave)
                writeWaveHeader(out, 0);

            trackSectorsWritten = 0;
        }

        if ((entry.fileIndex != -1) && (entry.fileIndex != currentFile))
        {
            currentFile = entry.fileIndex;

            if (in.isOpen())
                in.close();

            in.setFileName(toc->fileList().at(currentFile).fileName);
            if (!in.open(QIODevice::ReadOnly))
            {
                qCritical().noquote() << "Could not open input file: " << toc->fileList().at(currentFile).fileName << endl << in.errorString() << endl;
                break;
            }

            if (currentType == CdromToc::TrackType::AudioWav)
                inWave.initialize(&in);
        }

        if (entry.fileIndex != -1)
        {
            if (currentType == CdromToc::TrackType::AudioPCM)
            {
                if (!writePcmAudio(in, out, entry, sectorsProcessed))
                    break;
            }
            else if (currentType == CdromToc::TrackType::AudioWav)
            {
                if (!writeWaveAudio(inWave, out, entry, sectorsProcessed))
                    break;
            }
            else if (currentType == CdromToc::TrackType::Mode1_2048)
            {
                if (!writeIsoData(in, out, entry, sectorsProcessed))
                    break;
            }
            else if (currentType == CdromToc::TrackType::Mode1_2352)
            {
                if (!writeRawData(in, out, entry, sectorsProcessed))
                    break;
            }

            trackSectorsWritten += entry.trackLength;
        }

        sectorsProcessed += entry.trackLength;
    }

    if (out.isOpen())
    {
        if  (outFileIsWave)
            writeWaveHeader(out, trackSectorsWritten * CDROM_SECTOR_SIZE);

        out.close();
    }

    if (in.isOpen())
        in.close();

    emit finished();
}

void ImageWriterWorker::cancel()
{
    m_cancelFlag = true;
}

bool ImageWriterWorker::writeCueSheet(const QString &baseDirectory, const QString &baseName, CdromToc *toc)
{
    QFile outFile(buildOutputPath(baseDirectory, baseName, QStringLiteral("cue")));
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&outFile);
    out.setCodec("UTF-8");

    uint8_t currentTrack = 0;
    uint32_t currentSectorInFile = 0;

    for(const CdromToc::Entry& entry : toc->toc())
    {
        if (entry.trackIndex.track() != currentTrack)
        {
            currentTrack = entry.trackIndex.track();

            const CdromToc::Entry* firstEntry = toc->findTocEntry(TrackIndex{currentTrack, 1});
            if (!firstEntry)
            {
                qCritical().noquote() << "Internal error: Track " << currentTrack << " has no index 1!";
                return false;
            }

            QString fileType;
            QString trackType;
            QString suffix;

            if ((firstEntry->trackType == CdromToc::TrackType::Mode1_2048) || (firstEntry->trackType == CdromToc::TrackType::Mode1_2352))
            {
                fileType = QStringLiteral("BINARY");
                trackType =  QStringLiteral("MODE1/2048");
                suffix = QStringLiteral("iso");
            }
            else
            {
                fileType = QStringLiteral("WAVE");
                trackType = QStringLiteral("AUDIO");
                suffix = QStringLiteral("wav");
            }

            out << "FILE \"" << buildTrackOutputFilename(entry.trackIndex, baseName, suffix) << "\" " << fileType << endl;
            out << "  TRACK " << buildTrackNumber(entry.trackIndex) << " " << trackType << endl;

            currentSectorInFile = 0;
        }

        if ((entry.trackIndex.index() == 0) && (entry.fileIndex == -1))
            out << "    PREGAP " << buildMsf(entry.trackLength) << endl;
        else if (entry.fileIndex == -1)
            out << "    POSTGAP " << buildMsf(entry.trackLength) << endl;
        else
        {
            out << "    INDEX " << buildIndexNumber(entry.trackIndex) << " " << buildMsf(currentSectorInFile) << endl;
            currentSectorInFile += entry.trackLength;
        }
    }

    return true;
}

bool ImageWriterWorker::writePcmAudio(QFile &in, QFile &out, const CdromToc::Entry &entry, uint32_t progressValue)
{
    uint32_t length = entry.trackLength;

    in.seek(static_cast<qint64>(entry.fileOffset));

    while(length)
    {
        if (m_cancelFlag)
            return false;

        emit progressValueChanged(static_cast<int>(progressValue));
        QCoreApplication::processEvents();

        uint32_t slice = qMin(length, uint32_t(400));

        QByteArray data = in.read(slice * CDROM_SECTOR_SIZE);

        if (data.size() % CDROM_SECTOR_SIZE)
        {
            qCritical().noquote() << "Read error on input file: " << in.errorString();
            return false;
        }

        if (out.write(data) < data.size())
        {
            qCritical().noquote() << "Write error on output file: " << out.errorString();
            return false;
        }

        uint32_t count = static_cast<uint32_t>(data.size()) / CDROM_SECTOR_SIZE;
        progressValue += count;
        length -= count;
    }

    return true;
}

bool ImageWriterWorker::writeWaveAudio(WavFile &in, QFile &out, const CdromToc::Entry &entry, uint32_t progressValue)
{
    uint32_t length = entry.trackLength;

    in.seek(static_cast<qint64>(entry.fileOffset));

    QByteArray data(400 * CDROM_SECTOR_SIZE, Qt::Uninitialized);

    while(length)
    {
        if (m_cancelFlag)
            return false;

        emit progressValueChanged(static_cast<int>(progressValue));
        QCoreApplication::processEvents();

        uint32_t slice = qMin(length, uint32_t(400));

        qint64 reallyRead = in.read(data.data(), slice * CDROM_SECTOR_SIZE);

        if (reallyRead % CDROM_SECTOR_SIZE)
        {
            qCritical().noquote() << "Read error on input file.";
            return false;
        }

        if (out.write(data.constData(), reallyRead) < reallyRead)
        {
            qCritical().noquote() << "Write error on output file: " << out.errorString();
            return false;
        }

        uint32_t count = static_cast<uint32_t>(reallyRead) / CDROM_SECTOR_SIZE;
        progressValue += count;
        length -= count;
    }

    return true;
}

bool ImageWriterWorker::writeIsoData(QFile &in, QFile &out, const CdromToc::Entry &entry, uint32_t progressValue)
{
    uint32_t length = entry.trackLength;

    in.seek(static_cast<qint64>(entry.fileOffset));

    while(length)
    {
        if (m_cancelFlag)
            return false;

        emit progressValueChanged(static_cast<int>(progressValue));
        QCoreApplication::processEvents();

        uint32_t slice = qMin(length, uint32_t(400));

        QByteArray data = in.read(slice * CDROM_DATA_SIZE);

        if (data.size() % CDROM_DATA_SIZE)
        {
            qCritical().noquote() << "Read error on input file: " << in.errorString();
            return false;
        }

        if (out.write(data) < data.size())
        {
            qCritical().noquote() << "Write error on output file: " << out.errorString();
            return false;
        }

        uint32_t count = static_cast<uint32_t>(data.size()) / CDROM_DATA_SIZE;
        progressValue += count;
        length -= count;
    }

    return true;
}

bool ImageWriterWorker::writeRawData(QFile &in, QFile &out, const CdromToc::Entry &entry, uint32_t progressValue)
{
    uint32_t length = entry.trackLength;

    in.seek(static_cast<qint64>(entry.fileOffset));

    while(length)
    {
        if (m_cancelFlag)
            return false;

        emit progressValueChanged(static_cast<int>(progressValue));
        QCoreApplication::processEvents();

        uint32_t slice = qMin(length, uint32_t(400));

        QByteArray data = in.read(slice * CDROM_SECTOR_SIZE);

        if (data.size() % CDROM_SECTOR_SIZE)
        {
            qCritical().noquote() << "Read error on input file: " << in.errorString();
            return false;
        }

        uint32_t count = static_cast<uint32_t>(data.size()) / CDROM_SECTOR_SIZE;

        const char* ptr = data.constData();

        for(uint32_t i = 0; i < count; ++i)
        {
            if (!m_uncorrectedErrorsFlag)
            {
                if (!checkSectorData(ptr))
                {
                    qWarning().noquote() << "Data track contains uncorrected errors!";
                    m_uncorrectedErrorsFlag = true;
                }
            }

            if (out.write(ptr + CDROM_HEADER_SIZE, CDROM_DATA_SIZE) < CDROM_DATA_SIZE)
            {
                qCritical().noquote() << "Write error on output file: " << out.errorString();
                return false;
            }

            ptr += CDROM_SECTOR_SIZE;
        }

        progressValue += count;
        length -= count;
    }

    return true;
}

QString ImageWriterWorker::buildOutputPath(const QString &directory, const QString &baseName, const QString &suffix)
{
    return QStringLiteral("%1/%2.%3").arg(directory, baseName, suffix);
}

QString ImageWriterWorker::buildTrackNumber(const TrackIndex &trackIndex)
{
    return QStringLiteral("%1").arg(trackIndex.track(), 2, 10, QChar('0'));
}

QString ImageWriterWorker::buildIndexNumber(const TrackIndex &trackIndex)
{
    return QStringLiteral("%1").arg(trackIndex.index(), 2, 10, QChar('0'));
}

QString ImageWriterWorker::buildTrackOutputFilename(const TrackIndex& trackIndex, const QString &baseName, const QString &suffix)
{
    return QStringLiteral("%1-%2.%3").arg(buildTrackNumber(trackIndex), baseName, suffix);
}

QString ImageWriterWorker::buildTrackOutputPath(const QString &directory, const TrackIndex &trackIndex, const QString &baseName, const QString &suffix)
{
    return QStringLiteral("%1/%2").arg(directory, buildTrackOutputFilename(trackIndex, baseName, suffix));
}

QString ImageWriterWorker::buildMsf(uint32_t value)
{
    uint32_t m;
    uint32_t s;
    uint32_t f;
    CdromToc::toMSF(value, m, s, f);

    return QStringLiteral("%1:%2:%3")
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'))
            .arg(f, 2, 10, QChar('0'));
}

void ImageWriterWorker::writeWaveHeader(QFile &out, uint32_t dataSize)
{
    WaveRiffHeader riffHeader;
    WaveChunkHeader fmtHeader;
    WaveFmtChunk fmtChunk;
    WaveChunkHeader dataHeader;

    constexpr uint16_t audioFormat = 1;
    constexpr uint16_t channelCount = 2;
    constexpr uint32_t sampleRate = 44100;
    constexpr uint16_t bitsPerSample = 16;

    riffHeader.magic = 0x46464952;
    riffHeader.fileSize = dataSize + sizeof(fmtHeader) + sizeof(fmtChunk) + sizeof(dataHeader) + 4;
    riffHeader.formatId = 0x45564157;


    fmtHeader.magic = 0x20746d66;
    fmtHeader.dataSize = sizeof(fmtChunk);

    fmtChunk.audioFormat = audioFormat;
    fmtChunk.channelCount = channelCount;
    fmtChunk.sampleRate = sampleRate;
    fmtChunk.bitsPerSample = bitsPerSample;
    fmtChunk.bytesPerBlock = channelCount * (bitsPerSample / 8);
    fmtChunk.bytesPerSecond = sampleRate * fmtChunk.bytesPerBlock;

    dataHeader.magic = 0x61746164;
    dataHeader.dataSize = dataSize;

    out.seek(0);
    out.write(reinterpret_cast<const char *>(&riffHeader), sizeof(riffHeader));
    out.write(reinterpret_cast<const char *>(&fmtHeader), sizeof(fmtHeader));
    out.write(reinterpret_cast<const char *>(&fmtChunk), sizeof(fmtChunk));
    out.write(reinterpret_cast<const char *>(&dataHeader), sizeof(dataHeader));
}

uint32_t ImageWriterWorker::calculateEdc(const uint8_t *data, int len)
{
    uint32_t crc = 0;

    while (len--)
        crc = EDCTABLE[static_cast<int>(crc ^ *data++) & 0xff] ^ (crc >> 8);

    return crc;
}

bool ImageWriterWorker::checkSectorData(const void *data)
{
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);

    uint32_t checkEdc = calculateEdc(ptr, CDROM_DATA_SIZE + CDROM_HEADER_SIZE);
    const uint32_t* edc = reinterpret_cast<const uint32_t*>(ptr + CDROM_DATA_SIZE + CDROM_HEADER_SIZE);

    return (*edc == checkEdc);
}
