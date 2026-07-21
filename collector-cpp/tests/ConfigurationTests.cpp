#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#include "Configuration.h"
#include "Logger.h"

namespace sentinelforge {
namespace {

class ConfigurationTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        tempDir_ = std::filesystem::temp_directory_path() /
                   ("sf_cfg_test_" + std::string(info->name()));
        std::error_code ec;
        std::filesystem::remove_all(tempDir_, ec);
        std::filesystem::create_directories(tempDir_);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(tempDir_, ec);
    }

    std::filesystem::path WriteConfig(const std::string& contents) {
        const std::filesystem::path path = tempDir_ / "sentinelforge.json";
        std::ofstream out(path);
        out << contents;
        return path;
    }

    // Error-level logger: LoadFromFile requires a Logger, but these tests
    // assert only on returned Configuration values, never on log output. The
    // high threshold keeps the "missing file" warning out of test output.
    Logger quietLogger_{LogLevel::Error};
    std::filesystem::path tempDir_;
};

TEST_F(ConfigurationTest, DefaultConfigurationCreation) {
    const Configuration config = Configuration::Defaults();

    EXPECT_EQ(config.LoggingLevel(), LogLevel::Info) << "Default logging level should be INFO";
    EXPECT_EQ(config.ApiPort(), static_cast<std::uint16_t>(8080)) << "Default API port should be 8080";
    EXPECT_FALSE(config.DashboardEnabled()) << "Dashboard should be disabled by default";
    EXPECT_FALSE(config.RulesDirectory().empty()) << "Default rules directory should be set";
    EXPECT_FALSE(config.SampleEventFile().empty()) << "Default sample event file should be set";
    EXPECT_FALSE(config.OutputDirectory().empty()) << "Default output directory should be set";
}

TEST_F(ConfigurationTest, MissingConfigurationFileFallsBackToDefaults) {
    const std::filesystem::path missing = tempDir_ / "does_not_exist.json";
    ASSERT_FALSE(std::filesystem::exists(missing));

    const Configuration config = Configuration::LoadFromFile(missing, quietLogger_);
    const Configuration defaults = Configuration::Defaults();

    EXPECT_EQ(config.LoggingLevel(), defaults.LoggingLevel())
        << "A missing file should yield the default logging level";
    EXPECT_EQ(config.ApiPort(), defaults.ApiPort())
        << "A missing file should yield the default API port";
    EXPECT_EQ(config.DashboardEnabled(), defaults.DashboardEnabled())
        << "A missing file should yield the default dashboard setting";
    EXPECT_EQ(config.RulesDirectory(), defaults.RulesDirectory())
        << "A missing file should yield the default rules directory";
}

TEST_F(ConfigurationTest, InvalidLoggingLevelRejected) {
    const std::filesystem::path path = WriteConfig(R"({ "logging_level": "LOUD" })");

    EXPECT_THROW(Configuration::LoadFromFile(path, quietLogger_), ConfigurationError)
        << "An unknown logging level must be rejected";
}

TEST_F(ConfigurationTest, InvalidApiPortRejected) {
    const std::filesystem::path path = WriteConfig(R"({ "api_port": 70000 })");

    EXPECT_THROW(Configuration::LoadFromFile(path, quietLogger_), ConfigurationError)
        << "An out-of-range API port must be rejected";
}

TEST_F(ConfigurationTest, ImmutableGettersReturnExpectedValues) {
    const std::filesystem::path rulesDir = tempDir_ / "custom-rules";
    const std::filesystem::path sampleFile = tempDir_ / "custom-event.json";
    const std::filesystem::path outputDir = tempDir_ / "custom-output";

    const std::string json = std::string("{\n") +
        "  \"rules_directory\": \"" + rulesDir.generic_string() + "\",\n" +
        "  \"sample_event_file\": \"" + sampleFile.generic_string() + "\",\n" +
        "  \"output_directory\": \"" + outputDir.generic_string() + "\",\n" +
        "  \"logging_level\": \"WARNING\",\n" +
        "  \"api_port\": 1234,\n" +
        "  \"dashboard_enabled\": true\n" +
        "}\n";

    const Configuration config = Configuration::LoadFromFile(WriteConfig(json), quietLogger_);

    EXPECT_EQ(config.LoggingLevel(), LogLevel::Warning) << "logging_level should be read as WARNING";
    EXPECT_EQ(config.ApiPort(), static_cast<std::uint16_t>(1234)) << "api_port should be read as 1234";
    EXPECT_TRUE(config.DashboardEnabled()) << "dashboard_enabled should be read as true";
    EXPECT_EQ(config.RulesDirectory(), rulesDir) << "rules_directory getter should echo the file value";
    EXPECT_EQ(config.SampleEventFile(), sampleFile)
        << "sample_event_file getter should echo the file value";
    EXPECT_EQ(config.OutputDirectory(), outputDir)
        << "output_directory getter should echo the file value";
}

}  // namespace
}  // namespace sentinelforge
