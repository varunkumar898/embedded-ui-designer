#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QUrl>
#include <vector>

struct BoardSpec {
    QString id;
    QString name;              // "ESP32 DEVKITV1"
    QString manufacturer;      // "Espressif"
    QString family;
    QString mcuModel;          // "ESP32"
    QString mcu;               // alias
    QString architecture;
    int frequencyMhz = 0;
    double flashMb = 0.0;      // 4
    int flashKb = 0;           // 4096
    int ramKb = 0;             // 520
    int gpioPins = 0;          // 34
    int gpioCount = 0;         // 34
    QStringList analogPins;
    QStringList pwmPins;
    QStringList digitalPins;

    // USB / Debug Adapter
    QString usbAdapter;        // "CP2104"
    QString usbAdapterType;    // "CP2104"
    QString vid;
    QString pid;
    QString driver;

    // OpenOCD
    QString openOcdConfig;     // "target/esp32.cfg"
    QString openocdInterface;
    QString openocdTarget;
    QString openocdConfigFile;
    QString openocdTransport;

    // Toolchain & HAL
    QString toolchain;         // "esp-idf"
    QString compiler;
    QString halHeader;
    QString flags;

    // Datasheet & Links
    QString datasheetUrl;
    QString productUrl;
    QStringList features;

    bool isValid() const { return !id.isEmpty() || !name.isEmpty(); }
};

/**
 * @brief BoardDatabase loads board specifications from espboards.dev and local caches,
 * providing specs, pin configurations, OpenOCD config files, and datasheets.
 */
class BoardDatabase : public QObject {
    Q_OBJECT

public:
    static BoardDatabase& instance();

    explicit BoardDatabase(QObject* parent = nullptr);
    ~BoardDatabase() override = default;

    // Initialize from local JSON file
    bool loadFromFile(const QString& filePath);
    bool loadDatabase(const QString& jsonPath = "");

    // Attempt to load from espboards.dev API (fallback to local if fails)
    bool loadFromAPI(const QString& apiUrl = "https://www.espboards.dev/api/boards");
    bool syncFromEspboardsDev(const QString& apiUrl = "https://www.espboards.dev/api/boards.json");

    // Query boards
    std::vector<BoardSpec> getAllBoards() const;
    QList<BoardSpec> allBoards() const;
    int boardCount() const { return static_cast<int>(m_boards.size()); }

    BoardSpec getBoardByName(const QString& name) const;
    std::vector<BoardSpec> getBoardsByMCU(const QString& mcu) const;
    QStringList getBoardNames() const;

    BoardSpec findBoardById(const QString& id) const;
    BoardSpec findBoardByUsb(const QString& vid, const QString& pid) const;
    BoardSpec findBoardByName(const QString& name) const;
    BoardSpec findBoardByProbeName(const QString& probeName) const;

signals:
    void databaseUpdated();

private:
    void populateDefaults();
    bool parseJson(const QByteArray& jsonData);
    BoardSpec parseBoard(const class QJsonObject& obj);

    std::vector<BoardSpec> m_boards;
    QMap<QString, BoardSpec> m_boardsById;
    QMap<QString, BoardSpec> m_boardsByUsbKey; // "vid:pid"
};
