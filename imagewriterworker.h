#ifndef IMAGEWRITERWORKER_H
#define IMAGEWRITERWORKER_H

#include <QObject>
#include <QString>

#include "cdromtoc.h"
#include "wavfile.h"

class ImageWriterWorker : public QObject
{
    Q_OBJECT
public:
    explicit ImageWriterWorker(QObject *parent = Q_NULLPTR);
    virtual ~ImageWriterWorker() Q_DECL_OVERRIDE;

signals:
    void started();
    void finished();
    void progressRangeChanged(int minimum, int maximum);
    void progressTextChanged(const QString& text);
    void progressValueChanged(int value);

public slots:
    void start(const QString& baseDirectory, const QString& baseName, CdromToc* toc);

    void cancel();

protected:
    bool writeCueSheet(const QString &baseDirectory, const QString &baseName, CdromToc *toc);

    bool writePcmAudio(QFile& in, QFile& out, const CdromToc::Entry& entry, uint32_t progressValue);
    bool writeWaveAudio(WavFile& in, QFile& out, const CdromToc::Entry& entry, uint32_t progressValue);
    bool writeIsoData(QFile& in, QFile& out, const CdromToc::Entry& entry, uint32_t progressValue);
    bool writeRawData(QFile& in, QFile& out, const CdromToc::Entry& entry, uint32_t progressValue);

    static QString buildOutputPath(const QString& directory, const QString& baseName, const QString& suffix);
    static QString buildTrackNumber(const TrackIndex& trackIndex);
    static QString buildIndexNumber(const TrackIndex& trackIndex);
    static QString buildTrackOutputFilename(const TrackIndex& trackIndex, const QString& baseName, const QString &suffix);
    static QString buildTrackOutputPath(const QString& baseDirectory, const TrackIndex& trackIndex, const QString& baseName, const QString& suffix);
    static QString buildMsf(uint32_t value);
    static void writeWaveHeader(QFile& out, uint32_t dataSize);
    static uint32_t calculateEdc(const uint8_t *data, int len);
    static bool checkSectorData(const void* data);

    bool m_cancelFlag;
    bool m_uncorrectedErrorsFlag;
};

#endif // IMAGEWRITERWORKER_H
