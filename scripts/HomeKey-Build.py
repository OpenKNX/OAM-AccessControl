from pathlib import Path
import re
Import("env")
project = Path(env.subst("$PROJECT_DIR"))
# A stale Windows checkout built the old 23-byte sprintf prefix and rebooted
# at FlashDriver<fingerprint> before Home Key could start. The fixed Common
# revision is already pinned; reject the known unsafe implementation explicitly.
logger = project / "lib" / "OGM-Common" / "src" / "OpenKNX" / "Log" / "Logger.cpp"
logger_text = logger.read_text()
prefix_start = logger_text.find("std::string Logger::buildPrefix(")
prefix_end = logger_text.find("void Logger::beforeLog()", prefix_start)
if prefix_start < 0 or prefix_end < 0:
    raise RuntimeError("Cannot verify Common logger prefix safety: " + str(logger.resolve()))
prefix_code = re.sub(r"/\*.*?\*/|//[^\n]*", "", logger_text[prefix_start:prefix_end], flags=re.S)
if re.search(r"\b(?:sprintf|vsprintf)\s*\(", prefix_code):
    raise RuntimeError(
        "Unsafe OGM-Common logger: FlashDriver<fingerprint> overflows the old prefix buffer. "
        "Update lib/OGM-Common from private u20p17/OGM-Common, branch v1dev_HomeKey "
        "(fix 73bd379, pinned in dependencies.txt), then clean and rebuild. Checked: " + str(logger.resolve()))

header = project / "include" / "knxprod.h"
if not header.exists():
    raise RuntimeError("Generate the development ETS header from src/AccessControl.xml first")

# The ETS row decoder consumes a fixed byte layout. Fail before compiling if
# producer output no longer matches it (including bit positions and NUL space).
header_text = header.read_text()

def constant(name):
    match = re.search(r"^#define\s+" + re.escape(name) + r"\s+(0x[0-9a-fA-F]+|[0-9]+)\b", header_text, re.MULTILINE)
    if not match:
        raise RuntimeError("Missing Home Key ETS constant: " + name)
    return int(match.group(1), 0)

base = constant("HK_Authorization01Enabled")
for row in range(16):
    prefix = f"HK_Authorization{row + 1:02}"
    expected = {"Enabled": base + row * 33, "EnabledMask": 0x80,
                "Issuer": base + row * 33 + 1, "IssuerLength": 17,
                "Endpoint": base + row * 33 + 18, "EndpointLength": 13,
                "AccessId": base + row * 33 + 31}
    for suffix, value in expected.items():
        if constant(prefix + suffix) != value:
            raise RuntimeError("Home Key ETS row layout mismatch: " + prefix + suffix)
