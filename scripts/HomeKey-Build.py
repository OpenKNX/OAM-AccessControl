from pathlib import Path
Import("env")
header = Path(env.subst("$PROJECT_DIR")) / "include/homekey/knxprod.h"
if not header.exists():
    raise RuntimeError("Generate the Apple Home Key ETS header from src/AccessControl-HomeKey.xml first")
env.Prepend(CPPPATH=[str(header.parent)])
