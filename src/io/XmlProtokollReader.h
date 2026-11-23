#pragma once

#include "model/Entities.h"
#include <QSet>
#include <memory>

namespace RoboCell {

/**
 * @brief Liest einzelne XML-Schichtprotokolle mit QXmlStreamReader ein (Lazy Loading, Mindestanforderung 5 & 7).
 */
class XmlProtokollReader {
public:
    /**
     * @brief Parst eine XML-Schichtprotokolldatei und führt Plausibilitätsprüfungen durch.
     * @param filePath Vollständiger oder relativer Dateipfad zur XML-Datei
     * @param knownProducts Menge bekannter Produkt-IDs zur Plausibilitätsprüfung (optional)
     * @param knownStoercodes Menge bekannter Störcodes zur Plausibilitätsprüfung (optional)
     */
    static std::shared_ptr<Schichtprotokoll> readFromFile(
        const QString& filePath,
        const QSet<QString>& knownProducts = {},
        const QSet<QString>& knownStoercodes = {}
    );
};

} // namespace RoboCell
