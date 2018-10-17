#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QtDebug>
#include <algorithm>

#include "cdromtoc.h"
#include "wavfile.h"

static QString pathReplaceFilename(const QString& path, const QString& newFilename)
{
    return QFileInfo(QFileInfo(path).dir(), newFilename).filePath();
}

CdromToc::CdromToc() :
    m_toc(),
    m_fileList(),
    m_firstTrack(0),
    m_lastTrack(0),
    m_totalSectors(0)
{ }

bool CdromToc::loadCueSheet(const QString &filename)
{
    static const QRegularExpression FILE_REGEX("^\\s*FILE\\s+\"(.*)\"\\s+(\\S+)\\s*$", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression TRACK_REGEX("^\\s*TRACK\\s+([0-9]+)\\s+(\\S*)\\s*$", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression PREGAP_REGEX("^\\s*PREGAP\\s+([0-9]+):([0-9]+):([0-9]+)\\s*$", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression INDEX_REGEX("^\\s*INDEX\\s+([0-9]+)\\s+([0-9]+):([0-9]+):([0-9]+)\\s*$", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression POSTGAP_REGEX("^\\s*POSTGAP\\s+([0-9]+):([0-9]+):([0-9]+)\\s*$", QRegularExpression::CaseInsensitiveOption);

    m_toc.clear();
    m_fileList.clear();
    m_firstTrack = 0;
    m_lastTrack = 0;
    m_totalSectors = 0;

    QFile inFile(filename);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCritical().noquote() << "Could not open CUE file: " << inFile.errorString();
        return false;
    }

    QTextStream in(&inFile);
    in.setCodec("UTF-8");

    //***********************************
    // CUE parsing is done in three steps.
    //
    // For the first step:
    // - Check the CUE sheet syntax as thoroughly as possible
    // - Create the TOC structure, leaving some fields blanks for now
    // - Make a list of all source files, with their size. (Size of uncompressed audio for audio tracks)
    //***********************************

    QString currentFile;
    int currentFileIndex = -1;
    TrackType currentFileAudioType = TrackType::AudioPCM;
    int currentTrack = -1;
    int currentIndex = -1;
    TrackType currentType = TrackType::Silence;
    bool trackHasPregap = false;
    bool trackHasPostgap = false;
    bool trackHasIndexOne = false;

    while(!in.atEnd())
    {
        QString line = in.readLine();

        QRegularExpressionMatch match;

        match = FILE_REGEX.match(line);
        if (match.hasMatch())
        {
            currentFile = pathReplaceFilename(filename, match.captured(1));
            currentTrack = -1;
            currentIndex = -1;
            currentType = TrackType::Silence;
            trackHasPregap = false;
            trackHasPostgap = false;
            trackHasIndexOne = false;

            QString type = match.captured(2);
            bool isBinary = (type.compare("BINARY", Qt::CaseInsensitive) == 0);
            bool isWave = (type.compare("WAVE", Qt::CaseInsensitive) == 0);

            if (!isBinary && !isWave)
            {
                qCritical().noquote() << "File type " << type << " is not supported.";
                return false;
            }

            auto i = std::find_if(m_fileList.cbegin(), m_fileList.cend(), [&](const FileEntry& entry) { return entry.fileName == currentFile; });
            if (i == m_fileList.cend())
            {
                QFile file(currentFile);
                if (!file.open(QIODevice::ReadOnly))
                {
                    qCritical().noquote() << "File " << match.captured(1) << " could not be opened: " << file.errorString();
                    return false;
                }

                qint64 fileSize;

                if (isBinary)
                {
                    fileSize = file.size();
                    currentFileAudioType = TrackType::AudioPCM;
                }
                else
                {
                    if (!findAudioFileSize(file, fileSize, currentFileAudioType))
                        return false;
                }

                m_fileList.push_back({currentFile, fileSize});

                currentFileIndex = m_fileList.size() - 1;
            }
            else
                currentFileIndex = static_cast<int>(std::distance(m_fileList.cbegin(), i));

            continue;
        }

        match = TRACK_REGEX.match(line);
        if (match.hasMatch())
        {
            if (currentFileIndex < 0)
            {
                qCritical().noquote() << "Invalid CUE sheet: Track directive without file!";
                return false;
            }

            int newTrack =  match.captured(1).toInt();

            if ((newTrack < 1) || (newTrack > 99))
            {
                qCritical().noquote() << "Invalid CUE sheet: Track numbers should be between 1 and 99.";
                return false;
            }

            if ((currentTrack != -1) && (newTrack - currentTrack != 1))
            {
                qCritical().noquote() << "Invalid CUE sheet: Track numbers should be contiguous and increasing.";
                return false;
            }

            if ((currentTrack != -1) && (!trackHasIndexOne))
            {
                qCritical().noquote() << "Invalid CUE sheet: Track " << currentTrack << " has no index 01.";
                return false;
            }

            currentTrack = newTrack;
            currentIndex = -1;
            trackHasPregap = false;
            trackHasPostgap = false;
            trackHasIndexOne = false;

            QString mode = match.captured(2);

            if (mode.compare(QStringLiteral("MODE1/2048"), Qt::CaseInsensitive) == 0)
                currentType = TrackType::Mode1_2048;
            else if (mode.compare(QStringLiteral("MODE1/2352"), Qt::CaseInsensitive) == 0)
                currentType = TrackType::Mode1_2352;
            else if (mode.compare(QStringLiteral("AUDIO"), Qt::CaseInsensitive) == 0)
                currentType = currentFileAudioType;
            else
            {
                qCritical().noquote() << "Track mode " << match.captured(2) << " is not supported";
                return false;
            }

            if (((currentType == TrackType::Mode1_2048) || (currentType == TrackType::Mode1_2352)) && (currentFileAudioType != TrackType::AudioPCM))
            {
                qCritical().noquote() << "Invalid CUE sheet: Data track defined when current source file is audio type.";
                return false;
            }

            continue;
        }

        match = PREGAP_REGEX.match(line);
        if (match.hasMatch())
        {
            if (currentTrack < 0)
            {
                qCritical().noquote() << "Invalid CUE sheet: Pregap directive with no track defined.";
                return false;
            }

            if (trackHasPregap)
            {
                qCritical().noquote() << "Invalid CUE sheet: A track can have only one pregap.";
                return false;
            }

            if (currentIndex >= 0)
            {
                qCritical().noquote() << "Invalid CUE sheet: Pregap directive must come before any indexes.";
                return false;
            }

            uint32_t m = match.captured(1).toUInt();
            uint32_t s = match.captured(2).toUInt();
            uint32_t f = match.captured(3).toUInt();
            uint32_t length = fromMSF(m, s, f);

            m_toc.push_back({ -1, { static_cast<uint8_t>(currentTrack), 0 }, TrackType::Silence, 0, 0, 0, length });

            trackHasPregap = true;

            continue;
        }

        match = INDEX_REGEX.match(line);
        if (match.hasMatch())
        {
            if (currentTrack < 0)
            {
                qCritical().noquote() << "Invalid CUE sheet: Index directive with no track defined.";
                return false;
            }

            if (trackHasPostgap)
            {
                qCritical().noquote() << "Invalid CUE sheet: Index directive must come before postgap.";
                return false;
            }

            int newIndex = match.captured(1).toInt();

            if ((newIndex < 0) || (newIndex > 99))
            {
                qCritical().noquote() << "Invalid CUE sheet: Index numbers should be between 0 and 99.";
                return false;
            }

            if (trackHasPregap && (newIndex == 0))
            {
                qCritical().noquote() << "Invalid CUE sheet: Index 0 is not allowed with pregap.";
                return false;
            }

            if ((currentIndex != -1) && (newIndex - currentIndex != 1))
            {
                qCritical().noquote() << "Invalid CUE sheet: Index numbers should be contiguous and increasing.";
                return false;
            }

            uint32_t m = match.captured(2).toUInt();
            uint32_t s = match.captured(3).toUInt();
            uint32_t f = match.captured(4).toUInt();
            uint32_t indexPosition = fromMSF(m, s, f);

            currentIndex = newIndex;

            if (currentIndex == 1)
                trackHasIndexOne = true;

            m_toc.push_back({ currentFileIndex, { static_cast<uint8_t>(currentTrack), static_cast<uint8_t>(currentIndex) }, currentType, indexPosition, 0, 0, 0 });

            continue;
        }

        match = POSTGAP_REGEX.match(line);
        if (match.hasMatch())
        {
            if (currentTrack < 0)
            {
                qCritical().noquote() << "Invalid CUE sheet: Postgap directive with no track defined.";
                return false;
            }

            if (currentIndex < 0)
            {
                qCritical().noquote() << "Invalid CUE sheet: Postgap directive must come after all indexes.";
                return false;
            }

            if (trackHasPostgap)
            {
                qCritical().noquote() << "Invalid CUE sheet: A track can have only one postgap.";
                return false;
            }

            ++currentIndex;
            uint32_t m = match.captured(1).toUInt();
            uint32_t s = match.captured(2).toUInt();
            uint32_t f = match.captured(3).toUInt();
            uint32_t length = fromMSF(m, s, f);

            m_toc.push_back({ -1, { static_cast<uint8_t>(currentTrack), static_cast<uint8_t>(currentIndex) }, TrackType::Silence, 0, 0, 0, length });

            trackHasPostgap = true;

            continue;
        }
    }

    // Final syntax checks

    if (currentTrack == -1)
    {
        qCritical().noquote() << "Invalid CUE sheet: Should define at least one track.";
        return false;
    }

    if ((currentTrack != -1) && (!trackHasIndexOne))
    {
        qCritical().noquote() << "Invalid CUE sheet: Track " << currentTrack << " has no index 01.";
        return false;
    }

    //****************************
    // For step 2:
    //
    // - Calculate the size of every TOC entry
    // - Calculate the start offset of every TOC entry in the data files
    // ***************************

    // Build a list of pointers to every (track;index) couple
    QVector<CdromToc::Entry*> sortedList;
    for(CdromToc::Entry& entry : m_toc)
        sortedList.push_back(&entry);

    // Sort the list by file index and "start sector" (offset in the file)
    std::sort(sortedList.begin(), sortedList.end(), [](const CdromToc::Entry* left, const CdromToc::Entry* right) -> bool
    {
        if (left->fileIndex == right->fileIndex)
            return left->indexPosition < right->indexPosition;

        return left->fileIndex < right->fileIndex;
    });

    auto i = sortedList.begin();
    while(i != sortedList.end())
    {
        int fileIndex = (*i)->fileIndex;

        // Count the number of TOC entries belonging to the same file
        ptrdiff_t count = std::count_if(i, sortedList.end(), [&](CdromToc::Entry* entry) -> bool
        {
            return entry->fileIndex == fileIndex;
        });

        // Index -1 is for silence entries with no data, skip it
        if (fileIndex == -1)
        {
            std::advance(i, count);
            continue;
        }

        size_t currentFileOffset = 0;

        auto end = std::next(i, count);
        while(i != end)
        {
            auto next = std::next(i, 1);

            uint32_t trackLength = 0;
            size_t sectorSize = 0;

            // Find the sector size from enty type
            if ((*i)->trackType == TrackType::Mode1_2048)
                sectorSize = 2048;
            else
                sectorSize = 2352;

            if (next == end)
            {
                // Last TOC entry of the file, calculate from the file size instead
                trackLength = static_cast<uint32_t>((static_cast<size_t>(m_fileList.at(fileIndex).fileSize) - currentFileOffset) / sectorSize);
            }
            else
            {
                // Length is start of next TOC entry - start of this entry
                trackLength = (*next)->indexPosition - (*i)->indexPosition;
            }

            // Update the entry with the file offset and length
            (*i)->fileOffset = currentFileOffset;
            (*i)->trackLength = trackLength;

            // Update the file offset
            currentFileOffset += (trackLength * sectorSize);
            ++i;
        }
    }

    //***********************
    // For step three:
    //
    // - Calculate the position of everything on our virtual disc
    //***********************

    uint32_t currentSector = 0;

    for(CdromToc::Entry& entry : m_toc)
    {
        entry.startSector = currentSector;
        currentSector += entry.trackLength;
    }

    m_totalSectors = currentSector;
    m_firstTrack = m_toc.first().trackIndex.track() ;
    m_lastTrack = m_toc.last().trackIndex.track() ;

    return true;
}

const CdromToc::Entry *CdromToc::findTocEntry(const TrackIndex &trackIndex)
{
    auto i = std::lower_bound(m_toc.cbegin(), m_toc.cend(), trackIndex, [](const CdromToc::Entry& entry, const TrackIndex& trackIndex) -> bool
    {
        return entry.trackIndex < trackIndex;
    });

    if ((i == m_toc.cend()) || (i->trackIndex != trackIndex))
        return Q_NULLPTR;

    return i;
}

bool CdromToc::findAudioFileSize(QFile &file, qint64 &fileSize, TrackType &trackType)
{
    QFileInfo fileInfo(file.fileName());

    if (fileInfo.suffix().compare(QStringLiteral("WAV"), Qt::CaseInsensitive) == 0)
    {
        WavFile wavFile;

        if (!wavFile.initialize(&file))
        {
            qCritical().noquote() << "File " << fileInfo.fileName() << " is not a valid WAV file.";
            return false;
        }

        fileSize = wavFile.length();
        trackType = TrackType::AudioWav;

        wavFile.cleanup();

        return true;
    }
    qCritical().noquote() << "File type " << fileInfo.suffix() << " is not supported.";
    return false;
}


