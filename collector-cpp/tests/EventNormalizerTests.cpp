#include <cstdint>
#include <string>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "Event.h"
#include "EventNormalizer.h"
#include "NormalizedEvent.h"

namespace sentinelforge {
namespace {

class EventNormalizerTest : public ::testing::Test {
protected:
    EventNormalizer normalizer_;
};

TEST_F(EventNormalizerTest, SuccessfulNormalizationFromEvent) {
    const Event event("2026-07-20T14:32:07Z", "WORKSTATION-07", "CORP\\jsmith", "powershell.exe",
                      "explorer.exe", "powershell.exe -enc SGVsbG8=",
                      static_cast<std::uint32_t>(4821));

    const NormalizedEvent normalized = normalizer_.Normalize(event);

    EXPECT_EQ(normalized.Timestamp(), "2026-07-20T14:32:07Z");
    EXPECT_EQ(normalized.Hostname(), "WORKSTATION-07");
    EXPECT_EQ(normalized.Username(), "CORP\\jsmith");
    EXPECT_EQ(normalized.ProcessName(), "powershell.exe");
    EXPECT_EQ(normalized.ParentProcess(), "explorer.exe");
    EXPECT_EQ(normalized.CommandLine(), "powershell.exe -enc SGVsbG8=");
    EXPECT_EQ(normalized.Provider(), "json");
    EXPECT_TRUE(normalized.EventType().empty());
    EXPECT_TRUE(normalized.FilePath().empty());
    EXPECT_TRUE(normalized.Hash().empty());
    EXPECT_TRUE(normalized.SourceIp().empty());
    EXPECT_TRUE(normalized.DestinationIp().empty());
    EXPECT_TRUE(normalized.DestinationPort().empty());
    EXPECT_TRUE(normalized.Severity().empty());
}

TEST_F(EventNormalizerTest, MissingOptionalFieldsRemainEmpty) {
    const nlohmann::json json = {{"timestamp", "2026-01-01T00:00:00Z"},
                                 {"hostname", "HOST"},
                                 {"username", "user"},
                                 {"process_name", "cmd.exe"},
                                 {"parent_process", "explorer.exe"},
                                 {"command_line", "cmd.exe /c whoami"}};

    const NormalizedEvent normalized = normalizer_.NormalizeJson(json);

    EXPECT_EQ(normalized.ProcessName(), "cmd.exe");
    EXPECT_TRUE(normalized.FilePath().empty());
    EXPECT_TRUE(normalized.Hash().empty());
    EXPECT_TRUE(normalized.SourceIp().empty());
    EXPECT_TRUE(normalized.DestinationIp().empty());
    EXPECT_TRUE(normalized.DestinationPort().empty());
    EXPECT_TRUE(normalized.EventType().empty());
    EXPECT_TRUE(normalized.Severity().empty());
    EXPECT_EQ(normalized.Provider(), "json");
}

TEST_F(EventNormalizerTest, MissingRequiredFieldsRemainEmptyWithoutThrowing) {
    const nlohmann::json json = {{"hostname", "HOST-ONLY"}};

    const NormalizedEvent normalized = normalizer_.NormalizeJson(json);

    EXPECT_EQ(normalized.Hostname(), "HOST-ONLY");
    EXPECT_TRUE(normalized.Timestamp().empty());
    EXPECT_TRUE(normalized.Username().empty());
    EXPECT_TRUE(normalized.ProcessName().empty());
    EXPECT_TRUE(normalized.CommandLine().empty());
    EXPECT_EQ(normalized.Provider(), "json");
}

TEST_F(EventNormalizerTest, EmptyValuesArePreserved) {
    const Event event("", "", "", "", "", "", static_cast<std::uint32_t>(0));

    const NormalizedEvent normalized = normalizer_.Normalize(event);

    EXPECT_TRUE(normalized.Timestamp().empty());
    EXPECT_TRUE(normalized.Hostname().empty());
    EXPECT_TRUE(normalized.Username().empty());
    EXPECT_TRUE(normalized.ProcessName().empty());
    EXPECT_TRUE(normalized.ParentProcess().empty());
    EXPECT_TRUE(normalized.CommandLine().empty());
    EXPECT_EQ(normalized.Provider(), "json");
}

TEST_F(EventNormalizerTest, UnknownJsonFieldsAreIgnored) {
    const nlohmann::json json = {{"timestamp", "2026-07-20T14:32:07Z"},
                                 {"hostname", "WORKSTATION-07"},
                                 {"username", "CORP\\jsmith"},
                                 {"process_name", "powershell.exe"},
                                 {"parent_process", "explorer.exe"},
                                 {"command_line", "powershell.exe -enc SGVsbG8="},
                                 {"pid", 4821},
                                 {"totally_unknown_field", "should-be-ignored"},
                                 {"another_extra", 42}};

    const NormalizedEvent normalized = normalizer_.NormalizeJson(json);

    EXPECT_EQ(normalized.ProcessName(), "powershell.exe");
    EXPECT_EQ(normalized.CommandLine(), "powershell.exe -enc SGVsbG8=");
    EXPECT_EQ(normalized.Provider(), "json");
}

TEST_F(EventNormalizerTest, OptionalEnrichmentFieldsMappedFromJson) {
    const nlohmann::json json = {{"process_name", "malware.exe"},
                                 {"command_line", "malware.exe --drop"},
                                 {"event_type", "process_create"},
                                 {"file_path", "C:\\Temp\\malware.exe"},
                                 {"hash", "deadbeef"},
                                 {"source_ip", "10.0.0.5"},
                                 {"destination_ip", "8.8.8.8"},
                                 {"destination_port", "443"},
                                 {"severity", "high"},
                                 {"provider", "sysmon"}};

    const NormalizedEvent normalized = normalizer_.NormalizeJson(json);

    EXPECT_EQ(normalized.EventType(), "process_create");
    EXPECT_EQ(normalized.FilePath(), "C:\\Temp\\malware.exe");
    EXPECT_EQ(normalized.Hash(), "deadbeef");
    EXPECT_EQ(normalized.SourceIp(), "10.0.0.5");
    EXPECT_EQ(normalized.DestinationIp(), "8.8.8.8");
    EXPECT_EQ(normalized.DestinationPort(), "443");
    EXPECT_EQ(normalized.Severity(), "high");
    EXPECT_EQ(normalized.Provider(), "sysmon");
}

TEST_F(EventNormalizerTest, DestinationPortAcceptsNumericJson) {
    const nlohmann::json json = {{"destination_port", 8080}};

    const NormalizedEvent normalized = normalizer_.NormalizeJson(json);

    EXPECT_EQ(normalized.DestinationPort(), "8080");
}

TEST_F(EventNormalizerTest, NonObjectJsonYieldsEmptyNormalizedEvent) {
    const NormalizedEvent normalized = normalizer_.NormalizeJson(nlohmann::json::array());

    EXPECT_TRUE(normalized.ProcessName().empty());
    EXPECT_TRUE(normalized.Provider().empty());
}

}  // namespace
}  // namespace sentinelforge
