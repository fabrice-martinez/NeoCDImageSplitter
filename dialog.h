#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QFile>
#include <QIcon>
#include <QMap>
#include <cstdint>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = Q_NULLPTR);
    virtual ~Dialog() Q_DECL_OVERRIDE;

public slots:
    void loadCUE();
    void createSplitFileVersion();

private:
    bool parseCUE(const QString& cueFilename);

    bool outputSplitFiles(const QString& baseOutPath, const QString& baseOutFilename);

    bool writeIsoFile(QFile& inFile, const QString& outFilename, uint32_t sectorCount);

    bool writeWavFile(
            QFile& inFile,
            const QString& outFilename,
            uint32_t dataSize,
            uint16_t audioFormat,
            uint16_t channelCount,
            uint32_t sampleRate,
            uint16_t bitsPerSample);

    bool writeSplitCue(const QString& outFilename, const QString& baseFilename, const QString& audioExtension);

    bool isTrackDefined(int track);

    bool getTrackBounds(int track, uint32_t& startLBA, uint32_t& endLBA);

    void logMessage(const QString& msg);

    void updateDialog();

    inline Q_DECL_CONSTEXPR int makeTocIndex(int track, int index)
    {
        return (track << 8) | (index & 0xFF);
    }

    void lbaToMsf(uint32_t lba, uint32_t &m, uint32_t &s, uint32_t &f);

    uint32_t msfToLba(int m, int s, int f);

    QString buildMsfString(uint32_t lba);

    QString buildSplitPath(int track, const QString& basePath, const QString& baseFilename);
    QString buildSplitFilename(int track, const QString& baseFilename, const QString &extension);

    Ui::Dialog *ui;

    QIcon m_dataIcon;
    QIcon m_audioIcon;
    QIcon m_cdIcon;

    bool m_validTOCloaded;
    QString m_imageFilename;
    uint32_t m_leadout;
    QMap<int, uint32_t> m_toc;
};

#endif // DIALOG_H
