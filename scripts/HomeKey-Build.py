from pathlib import Path
import re
Import("env")
header = Path(env.subst("$PROJECT_DIR")) / "include" / "knxprod.h"
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
