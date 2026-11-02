param(
    [string]$Compiler = "g++"
)

$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$loggerPath = Join-Path $root "lib\OGM-Common\src\OpenKNX\Log\Logger.cpp"
$logger = Get-Content $loggerPath -Raw
$start = $logger.IndexOf("        std::string Logger::buildPrefix(")
$end = $logger.IndexOf("        void Logger::beforeLog()", $start)
if ($start -lt 0 -or $end -le $start) {
    throw "Cannot locate Logger::buildPrefix implementations"
}
$implementations = $logger.Substring($start, $end - $start)
$source = @'
#include <cassert>
#include <climits>
#include <cstdio>
#include <string>

#define OPENKNX_MAX_LOG_PREFIX_LENGTH 23

namespace OpenKNX { namespace Log {
class Logger {
public:
    std::string buildPrefix(const char*, const char*);
    std::string buildPrefix(const std::string&, const std::string&);
    std::string buildPrefix(const char*, int);
    std::string buildPrefix(const std::string&, int);
};
__IMPLEMENTATIONS__
}}

int main() {
    OpenKNX::Log::Logger logger;
    assert(logger.buildPrefix("FlashDriver", "fingerprint") == "FlashDriver<fingerprint>");
    assert(logger.buildPrefix("FlashDriver", "knx") == "FlashDriver<knx>");
    assert(logger.buildPrefix("", "") == "<>");
    const std::string prefix(1024, 'p');
    const std::string id(1024, 'i');
    assert(logger.buildPrefix(prefix.c_str(), id.c_str()) == prefix + "<" + id + ">");
    assert(logger.buildPrefix(prefix, id) == prefix + "<" + id + ">");
    for (int value : {0, 1, -1, INT_MIN, INT_MAX}) {
        const auto expected = prefix + "<" + std::to_string(value) + ">";
        assert(logger.buildPrefix(prefix.c_str(), value) == expected);
        assert(logger.buildPrefix(prefix, value) == expected);
    }
    std::puts("PASS: actual Logger::buildPrefix overloads, long IDs and integer limits");
}
'@
$source = $source.Replace("__IMPLEMENTATIONS__", $implementations)
$directory = Join-Path ([System.IO.Path]::GetTempPath()) ("oam-logger-" + [guid]::NewGuid().ToString("N"))
$null = New-Item -ItemType Directory -Path $directory
$sourcePath = Join-Path $directory "prefix_test.cpp"
$binaryPath = Join-Path $directory "prefix_test.exe"
try {
    [System.IO.File]::WriteAllText($sourcePath, $source)
    & $Compiler -std=c++17 -Wall -Wextra -Werror -fstack-protector-all $sourcePath -o $binaryPath
    if ($LASTEXITCODE -ne 0) { throw "Logger regression test compilation failed" }
    & $binaryPath
    if ($LASTEXITCODE -ne 0) { throw "Logger regression test failed" }
}
finally {
    if (Test-Path $binaryPath) { Remove-Item -LiteralPath $binaryPath }
    Remove-Item -LiteralPath $sourcePath -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $directory
}
