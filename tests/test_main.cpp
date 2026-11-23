#include "io/DataRepository.h"
#include "logic/OeeCalculator.h"
#include "core/AuthManager.h"
#include "core/LanguageManager.h"
#include <QCoreApplication>
#include <QTextStream>
#include <QDir>
#include <QFile>
#include <cassert>
#include <cmath>

using namespace RoboCell;

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    out << "=== RoboCell Automated Verification Suite ===\n";

    // 1. Test DataRepository & CSV Plausibility Checks
    DataRepository repo;
    const QString dataDir = DataRepository::findDefaultDataDirectory();
    out << "Found data directory: " << dataDir << "\n";

    const bool loaded = repo.loadAllFromDirectory(dataDir);
    assert(loaded && "DataRepository failed to load CSV files!");

    out << "Loaded Linien: " << repo.linien().size() << "\n";
    out << "Loaded Unique Anlagen: " << repo.anlagen().size() << "\n";
    out << "Loaded Produkte: " << repo.produkte().size() << "\n";
    out << "Loaded Unique Stoercodes: " << repo.stoercodes().size() << "\n";
    out << "Loaded Wartungen: " << repo.wartungen().size() << "\n";
    out << "Detected CSV Plausibility Issues: " << repo.validationIssues().size() << "\n";

    assert(repo.linien().size() == 6);
    assert(repo.anlagen().size() == 82);     // 83 rows minus 1 duplicate (A061)
    assert(repo.stoercodes().size() == 24);  // 25 rows minus 1 duplicate (M-102)
    assert(repo.validationIssues().size() >= 9);

    // Check specific anomaly handling:
    // A032 had "33,2" -> should be parsed as 33.2
    const Anlage* a032 = repo.findAnlage("A032");
    assert(a032 != nullptr);
    assert(std::abs(a032->sollTaktzeitS - 33.2) < 0.01);

    // A060 had 11 columns (missing traglast_kg) -> should be parsed with baujahr=2018, soll_taktzeit_s=67.7
    const Anlage* a060 = repo.findAnlage("A060");
    assert(a060 != nullptr);
    assert(a060->baujahr == 2018);
    assert(std::abs(a060->sollTaktzeitS - 67.7) < 0.01);

    // 2. Test Lazy Loading & OEE Calculation on valid XML (A001 on 2026-09-14)
    auto pA001 = repo.getProtokollLazy("A001", QDate(2026, 9, 14));
    assert(pA001 != nullptr && pA001->loadSuccess);
    const Anlage* a001 = repo.findAnlage("A001");
    assert(a001 != nullptr);

    OeeMetrics m001 = OeeCalculator::calculateDailyOee(*pA001, a001->sollTaktzeitS);
    assert(m001.isValid);
    assert(m001.verfuegbarkeit > 0.5 && m001.verfuegbarkeit <= 1.0);
    assert(m001.leistungsgrad > 0.5 && m001.leistungsgrad <= 1.2);
    assert(m001.qualitaetsrate > 0.8 && m001.qualitaetsrate <= 1.0);
    assert(m001.oee > 0.4 && m001.oee <= 1.0);
    out << QString("A001 (2026-09-14) -> V=%1%, L=%2%, Q=%3%, OEE=%4%, Good=%5, Scrap=%6, MTBF=%7m, MTTR=%8m\n")
            .arg(m001.verfuegbarkeit * 100.0, 0, 'f', 1)
            .arg(m001.leistungsgrad * 100.0, 0, 'f', 1)
            .arg(m001.qualitaetsrate * 100.0, 0, 'f', 1)
            .arg(m001.oee * 100.0, 0, 'f', 1)
            .arg(m001.gutteile)
            .arg(m001.ausschuss)
            .arg(m001.mtbfMinuten, 0, 'f', 1)
            .arg(m001.mttrMinuten, 0, 'f', 1);

    // 3. Test Graceful Degradation on Missing XML (A062 on 2026-09-22) and Malformed XML (A045 on 2026-09-25)
    auto pMissing = repo.getProtokollLazy("A062", QDate(2026, 9, 22));
    assert(pMissing != nullptr && !pMissing->loadSuccess);
    out << "Graceful handling of missing XML (A062_2026-09-22): " << pMissing->errorMessage << "\n";

    auto pMalformed = repo.getProtokollLazy("A045", QDate(2026, 9, 25));
    assert(pMalformed != nullptr && !pMalformed->loadSuccess);
    out << "Graceful handling of malformed XML (A045_2026-09-25): " << pMalformed->errorMessage << "\n";

    // 4. Test XML Semantic Plausibility Warnings (A007 backwards time, A009 negative parts)
    auto pA007 = repo.getProtokollLazy("A007", QDate(2026, 9, 21));
    assert(pA007 != nullptr && pA007->loadSuccess && !pA007->warnings.isEmpty());
    out << "Detected semantic warning in A007_2026-09-21: " << pA007->warnings.first() << "\n";

    auto pA009 = repo.getProtokollLazy("A009", QDate(2026, 9, 17));
    assert(pA009 != nullptr && pA009->loadSuccess && !pA009->warnings.isEmpty());
    out << "Detected semantic warning in A009_2026-09-17: " << pA009->warnings.first() << "\n";

    // 5. Test Line Bottleneck Analysis & Pareto
    auto bnL1 = OeeCalculator::analyzeLineBottlenecks("L1", repo, QDate(2026, 9, 14));
    assert(!bnL1.isEmpty());
    out << "L1 Bottleneck analysis stations: " << bnL1.size() << "\n";

    // 6. Test Master Data Export (anlagen_out.csv, wartungen_out.csv)
    QString exportErr;
    const bool exported = repo.exportModifiedCsv(dataDir, &exportErr);
    assert(exported && QFile::exists(QDir(dataDir).filePath("anlagen_out.csv")));
    out << "Successfully verified CSV export (anlagen_out.csv & wartungen_out.csv).\n";

    // 7. Test AuthManager & LanguageManager
    assert(AuthManager::instance().login("mitarbeiter", "1234"));
    assert(!AuthManager::instance().canEditMasterData());
    assert(AuthManager::instance().login("instandhaltung", "1234"));
    assert(AuthManager::instance().canRecordMaintenance());
    assert(AuthManager::instance().login("admin", "admin"));
    assert(AuthManager::instance().canEditMasterData());

    LanguageManager::instance().setLanguage(AppLanguage::English);
    assert(trBi("Deutsch", "English") == "English");
    LanguageManager::instance().setLanguage(AppLanguage::German);
    assert(trBi("Deutsch", "English") == "Deutsch");

    out << "=== ALL TESTS PASSED SUCCESSFULLY ===\n";
    return 0;
}
