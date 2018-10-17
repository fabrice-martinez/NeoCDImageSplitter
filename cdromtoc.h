#ifndef CDROMTOC_H
#define CDROMTOC_H

#include <QFile>
#include <QString>
#include <QVector>
#include <cstdint>

#include "trackindex.h"

class CdromToc
{
public:
    /// Enum representing all track types supported
    enum class TrackType
    {
        Mode1_2352,  /// Raw track (2352 bytes per sector)
        Mode1_2048,  /// ISO track (2048 bytes per sector)
        Silence,     /// Audio silence (no associated data)
        AudioPCM,    /// PCM audio (raw track)
        AudioFlac,   /// FLAC audio
        AudioOgg,    /// Ogg audio
        AudioWav     /// WAV audio
    };

    struct Entry
    {
        /// Index of file holding the track data in the file list
        int fileIndex;

        /// Track and index numbers for the entry
        TrackIndex trackIndex;

        /// Track type (MODE1/2048, MODE1/2352, AUDIO, etc)
        CdromToc::TrackType trackType;

        /// Index position in data file (sector)
        uint32_t indexPosition;

        /// Starting sector
        uint32_t startSector;

        /// Offset to the data of the track in the file (in bytes)
        size_t fileOffset;

        /// Track length (in sectors)
        uint32_t trackLength;
    };

    struct FileEntry
    {
        /// Path to the data file
        QString fileName;

        /// Size of the file, in bytes. For audio files this the size of uncompressed audio.
        qint64 fileSize;
    };

    explicit CdromToc();

    bool loadCueSheet(const QString& filename);

    inline const QVector<CdromToc::Entry>& toc() const
    {
        return m_toc;
    }

    const CdromToc::Entry* findTocEntry(const TrackIndex& trackIndex);

    inline const QVector<CdromToc::FileEntry>& fileList() const
    {
        return m_fileList;
    }

    inline uint32_t totalSectors() const
    {
        return m_totalSectors;
    }

    inline uint8_t lastTrack() const
    {
        return m_lastTrack;
    }

    inline uint8_t firstTrack() const
    {
        return m_firstTrack;
    }

    /**
     * @brief Converts a logical block address to sector number.
     * @param lba Logical Block Address.
     */
    static inline constexpr uint32_t fromLBA(uint32_t lba)
    {
        return (lba + 150);
    }

    /**
     * @brief Convert a sector number to logical block address.
     * @param position Sector number.
     */
    static inline constexpr uint32_t toLBA(uint32_t position)
    {
        return (position - 150);
    }

    /**
     * @brief Convert a sector number to minutes, seconds, frames.
     * @param[in] position The position to convert.
     * @param[out] m Minutes.
     * @param[out] s Seconds.
     * @param[out] f Frames.
     */
    static inline void toMSF(uint32_t position, uint32_t& m, uint32_t& s, uint32_t& f)
    {
        m = position / 4500;
        s = (position / 75) % 60;
        f = position % 75;
    }

    /**
     * @brief Convert a position in minutes, seconds, frames to a sector number
     * @param m Minutes.
     * @param s Seconds.
     * @param f Frames.
     * @return Sector Number.
     */
    static inline constexpr uint32_t fromMSF(uint32_t m, uint32_t s, uint32_t f)
    {
        return (m * 4500) + (s * 75) + f;
    }

protected:
    bool findAudioFileSize(QFile& file, qint64& fileSize, TrackType& trackType);

    QVector<CdromToc::Entry> m_toc;
    QVector<CdromToc::FileEntry> m_fileList;
    uint8_t m_firstTrack;
    uint8_t m_lastTrack;
    uint32_t m_totalSectors;
};

#endif // CDROMTOC_H
