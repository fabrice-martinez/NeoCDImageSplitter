#ifndef WAVFILE_H
#define WAVFILE_H

#include <QFile>

class WavFile
{
public:
    WavFile();
    ~WavFile();

    // Non copyable
    WavFile(const WavFile&) = delete;

    // Non copyable
    WavFile& operator=(const WavFile&) = delete;

    bool initialize(QFile* file);

    qint64 read(char *data, qint64 size);

    bool seek(qint64 position);

    qint64 length();

    void cleanup();

protected:
    QFile* m_file;
    qint64 m_currentPosition;
    qint64 m_dataStart;
    qint64 m_dataSize;
};

#endif // WAVFILE_H
