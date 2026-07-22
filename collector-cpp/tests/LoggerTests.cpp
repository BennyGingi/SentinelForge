#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "Logger.h"

namespace sentinelforge {
namespace {

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        tempDir_ = std::filesystem::temp_directory_path() /
                   ("sf_log_test_" + std::string(info->name()));
        std::error_code ec;
        std::filesystem::remove_all(tempDir_, ec);
        std::filesystem::create_directories(tempDir_);
        logPath_ = tempDir_ / "sentinelforge.log";
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(tempDir_, ec);
    }

    LoggingSettings FileOnly(LogLevel level) const {
        LoggingSettings settings;
        settings.level = level;
        settings.console = false;
        settings.file = true;
        settings.path = logPath_;
        return settings;
    }

    std::string ReadLogFile() const {
        std::ifstream in(logPath_);
        std::ostringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

    std::filesystem::path tempDir_;
    std::filesystem::path logPath_;
};

TEST_F(LoggerTest, LogLevelFiltering) {
    Logger logger(FileOnly(LogLevel::Info));

    logger.Debug("Application", "should be filtered");
    logger.Info("Application", "should appear");
    logger.Warn("Application", "should also appear");

    const std::string contents = ReadLogFile();
    EXPECT_EQ(contents.find("should be filtered"), std::string::npos)
        << "DEBUG must be suppressed when level is INFO";
    EXPECT_NE(contents.find("should appear"), std::string::npos)
        << "INFO must be emitted when level is INFO";
    EXPECT_NE(contents.find("should also appear"), std::string::npos)
        << "WARN must be emitted when level is INFO";
}

TEST_F(LoggerTest, TimestampExists) {
    Logger logger(FileOnly(LogLevel::Info));
    logger.Info("Application", "timestamp check");

    const std::string contents = ReadLogFile();
    const std::regex timestamp(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3} )");
    EXPECT_TRUE(std::regex_search(contents, timestamp))
        << "Every line must begin with YYYY-MM-DD HH:MM:SS.mmm; got:\n" << contents;
}

TEST_F(LoggerTest, ComponentNameFormatting) {
    Logger logger(FileOnly(LogLevel::Info));
    logger.Info("RuleLoader", "Loaded 3 rules.");

    const std::string contents = ReadLogFile();
    EXPECT_NE(contents.find("[INFO ] [RuleLoader] Loaded 3 rules."), std::string::npos)
        << "Line must include padded level and bracketed component; got:\n" << contents;
}

TEST_F(LoggerTest, FileOutputCreation) {
    ASSERT_FALSE(std::filesystem::exists(logPath_));

    const std::filesystem::path nested = tempDir_ / "nested" / "dir" / "app.log";
    LoggingSettings settings = FileOnly(LogLevel::Info);
    settings.path = nested;

    {
        Logger logger(settings);
        logger.Info("Application", "creates directories and file");
    }

    ASSERT_TRUE(std::filesystem::exists(nested)) << "Log file and parent directories must be created";

    // Append, never overwrite: a second logger writing the same path must keep the first line.
    {
        Logger logger(settings);
        logger.Info("Application", "second write");
    }

    const std::string contents = [&nested]() {
        std::ifstream in(nested);
        std::ostringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }();

    EXPECT_NE(contents.find("creates directories and file"), std::string::npos);
    EXPECT_NE(contents.find("second write"), std::string::npos);
}

TEST_F(LoggerTest, ConsolePlusFileConfiguration) {
    LoggingSettings settings;
    settings.level = LogLevel::Info;
    settings.console = true;
    settings.file = true;
    settings.path = logPath_;

    Logger logger(settings);
    EXPECT_TRUE(logger.ConsoleEnabled());
    EXPECT_TRUE(logger.FileEnabled());
    EXPECT_EQ(logger.FilePath(), logPath_);

    logger.Info("Application", "dual destination");

    const std::string contents = ReadLogFile();
    EXPECT_NE(contents.find("dual destination"), std::string::npos)
        << "File destination must receive the message when both are enabled";
    EXPECT_NE(contents.find("[Application]"), std::string::npos);
}

TEST_F(LoggerTest, InvalidConfigurationFallback) {
    LoggingSettings neither;
    neither.level = LogLevel::Info;
    neither.console = false;
    neither.file = false;
    neither.path = logPath_;

    Logger logger(neither);
    EXPECT_TRUE(logger.ConsoleEnabled())
        << "When both destinations are off, console must be forced on";
    EXPECT_FALSE(logger.FileEnabled()) << "File should remain disabled when not requested";

    LoggingSettings emptyPath;
    emptyPath.level = LogLevel::Info;
    emptyPath.console = false;
    emptyPath.file = true;
    emptyPath.path.clear();

    Logger fallback(emptyPath);
    EXPECT_TRUE(fallback.ConsoleEnabled())
        << "An empty file path must fall back to console output";
    EXPECT_FALSE(fallback.FileEnabled()) << "File output must be disabled when the path is empty";
}

TEST_F(LoggerTest, ThreadSafeConcurrentLogging) {
    constexpr int kThreads = 8;
    constexpr int kMessagesPerThread = 50;

    Logger logger(FileOnly(LogLevel::Info));
    std::vector<std::thread> threads;
    threads.reserve(static_cast<std::size_t>(kThreads));

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kMessagesPerThread; ++i) {
                logger.Info("Application",
                            "thread=" + std::to_string(t) + " msg=" + std::to_string(i));
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    const std::string contents = ReadLogFile();
    const std::regex linePattern(
        R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3} \[INFO \] \[Application\] thread=\d+ msg=\d+$)");

    std::size_t lineCount = 0;
    std::istringstream stream(contents);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }
        EXPECT_TRUE(std::regex_match(line, linePattern))
            << "Concurrent writes must not interleave; bad line: " << line;
        ++lineCount;
    }

    EXPECT_EQ(lineCount, static_cast<std::size_t>(kThreads * kMessagesPerThread))
        << "Every concurrent message must appear as exactly one intact line";
}

TEST_F(LoggerTest, ShouldLogAvoidsWorkWhenFiltered) {
    Logger logger(FileOnly(LogLevel::Error));
    EXPECT_FALSE(logger.ShouldLog(LogLevel::Debug));
    EXPECT_FALSE(logger.ShouldLog(LogLevel::Info));
    EXPECT_TRUE(logger.ShouldLog(LogLevel::Error));
    EXPECT_TRUE(logger.ShouldLog(LogLevel::Fatal));
}

}  // namespace
}  // namespace sentinelforge
