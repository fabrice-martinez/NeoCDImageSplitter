#include "endian.h"
#include "wavfile.h"
#include "wavstruct.h"

#include <algorithm>
#include <cassert>

WavFile::WavFile() :
    m_file(nullptr),
    m_currentPosition(0),
    m_dataStart(0),
    m_dataSize(0)
{
}

WavFile::~WavFile()
{
}

bool WavFile::initialize(QFile *file)
{
    cleanup();

    m_file = file;

    if (!m_file->isOpen())
        return false;

    WaveRiffHeader waveHeader;
    WaveChunkHeader chunkHeader;
    WaveFmtChunk fmtHeader;

    // First read in the header
    if (file->read(reinterpret_cast<char*>(&waveHeader), sizeof(waveHeader)) < static_cast<qint64>(sizeof(waveHeader)))
        return false;

    // ... and check that indeed it is a WAVE file
    if ((waveHeader.magic != LITTLE_ENDIAN_DWORD(0x46464952)) || (waveHeader.formatId != LITTLE_ENDIAN_DWORD(0x45564157)))
        return false;

    size_t fileSize = LITTLE_ENDIAN_DWORD(waveHeader.fileSize) + 8;
    qint64 fmtHeaderPos = 0;
    qint64 dataPos = 0;
    qint64 dataSize = 0;

    // Next, scan all chunks in the WAVE file looking for two specific chunks: fmt and data
    do
    {
        if (file->read(reinterpret_cast<char*>(&chunkHeader), sizeof(chunkHeader)) < static_cast<qint64>(sizeof(chunkHeader)))
            return false;

        qint64 currentPosition = file->pos();
        if (currentPosition < 0)
            return false;

        if (static_cast<size_t>(currentPosition) + LITTLE_ENDIAN_DWORD(chunkHeader.dataSize) > fileSize)
            return false;

        if (chunkHeader.magic == LITTLE_ENDIAN_DWORD(0x20746d66))
            fmtHeaderPos = currentPosition;
        else if (chunkHeader.magic == LITTLE_ENDIAN_DWORD(0x61746164))
        {
            dataPos = currentPosition;
            dataSize = LITTLE_ENDIAN_DWORD(chunkHeader.dataSize);
        }

        file->skip(LITTLE_ENDIAN_DWORD(chunkHeader.dataSize));
    } while((dataPos == 0) || (fmtHeaderPos == 0));

    // Read in the fmt chunk
    file->seek(fmtHeaderPos);
    if (file->read(reinterpret_cast<char*>(&fmtHeader), sizeof(fmtHeader)) < static_cast<qint64>(sizeof(fmtHeader)))
        return false;

    // ... and check that the audio has the desired format
    if ((fmtHeader.audioFormat != LITTLE_ENDIAN_WORD(1))
            || (fmtHeader.bitsPerSample != LITTLE_ENDIAN_WORD(16))
            || (fmtHeader.sampleRate != LITTLE_ENDIAN_DWORD(44100)))
        return false;

    // All ok!
    m_currentPosition = 0;
    m_dataStart = dataPos;
    m_dataSize = dataSize;

    file->seek(m_dataStart);

    return true;
}

qint64 WavFile::read(char *data, qint64 size)
{
    if ((!m_file) || (m_dataSize <= 0))
        return 0;

    qint64 available = m_dataSize - m_currentPosition;
    qint64 slice = std::min(size, available);

    qint64 done = m_file->read(data, slice);

    m_currentPosition += done;

    return done;
}

bool WavFile::seek(qint64 position)
{
    m_currentPosition = std::min(position, m_dataSize);

    m_file->seek(m_currentPosition + m_dataStart);

    return true;
}

qint64 WavFile::length()
{
    if ((!m_file) || (m_dataSize <= 0))
        return 0;

    return m_dataSize;
}

void WavFile::cleanup()
{
    m_currentPosition = 0;
    m_dataStart = 0;
    m_dataSize = 0;
}
