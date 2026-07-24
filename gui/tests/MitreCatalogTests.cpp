#include <gtest/gtest.h>

#include "mitre/MitreCatalog.h"

using sentinelforge::MitreCatalog;

TEST(MitreCatalogTest, AcceptsBareTechniqueId) {
    EXPECT_TRUE(MitreCatalog::isValidTechniqueId(QStringLiteral("T1059")));
}

TEST(MitreCatalogTest, AcceptsSubTechniqueId) {
    EXPECT_TRUE(MitreCatalog::isValidTechniqueId(QStringLiteral("T1059.001")));
}

TEST(MitreCatalogTest, RejectsShortDigitRun) {
    EXPECT_FALSE(MitreCatalog::isValidTechniqueId(QStringLiteral("T99")));
}

TEST(MitreCatalogTest, RejectsLongDigitRun) {
    EXPECT_FALSE(MitreCatalog::isValidTechniqueId(QStringLiteral("T10590")));
}

TEST(MitreCatalogTest, RejectsFourDigitSubTechnique) {
    EXPECT_FALSE(MitreCatalog::isValidTechniqueId(QStringLiteral("T1059.0001")));
}

TEST(MitreCatalogTest, RejectsEmptyString) {
    EXPECT_FALSE(MitreCatalog::isValidTechniqueId(QString()));
}

TEST(MitreCatalogTest, RejectsPathTraversalPayload) {
    EXPECT_FALSE(MitreCatalog::isValidTechniqueId(QStringLiteral("../../evil")));
}

TEST(MitreCatalogTest, RejectsTrailingGarbage) {
    EXPECT_FALSE(MitreCatalog::isValidTechniqueId(QStringLiteral("T1059.001 extra")));
}

// attackUrl() must refuse to build a URL from anything isValidTechniqueId rejects —
// this is the control that stops a malformed rule field from producing an arbitrary
// QDesktopServices::openUrl target.
TEST(MitreCatalogTest, InvalidIdProducesEmptyUrl) {
    EXPECT_TRUE(MitreCatalog::attackUrl(QStringLiteral("../../evil")).isEmpty());
}

TEST(MitreCatalogTest, ValidSubTechniqueIdBuildsMitreUrl) {
    const QUrl url = MitreCatalog::attackUrl(QStringLiteral("T1059.001"));
    EXPECT_EQ(url.toString(), QStringLiteral("https://attack.mitre.org/techniques/T1059/001/"));
}

TEST(MitreCatalogTest, ValidBareIdBuildsMitreUrl) {
    const QUrl url = MitreCatalog::attackUrl(QStringLiteral("T1059"));
    EXPECT_EQ(url.toString(), QStringLiteral("https://attack.mitre.org/techniques/T1059/"));
}
