"""Reject the logger overflow implementation before PlatformIO compiles it."""
from pathlib import Path
import runpy
import tempfile
import shutil

root = Path(__file__).resolve().parents[2]
script = root / 'scripts/HomeKey-Build.py'
logger = root / 'lib/OGM-Common/src/OpenKNX/Log/Logger.cpp'

class Environment:
    def __init__(self, project): self.project = project
    def subst(self, value):
        assert value == '$PROJECT_DIR'
        return str(self.project)

with tempfile.TemporaryDirectory(prefix='oam-build-guard-') as directory:
    project = Path(directory)
    target = project / 'lib/OGM-Common/src/OpenKNX/Log/Logger.cpp'
    target.parent.mkdir(parents=True)
    (project / 'include').mkdir()
    shutil.copyfile(root / 'include/knxprod.h', project / 'include/knxprod.h')
    target.write_text(logger.read_text())
    context = {'Import': lambda name: None, 'env': Environment(project)}
    runpy.run_path(str(script), init_globals=context)
    target.write_text('''std::string Logger::buildPrefix(const char* prefix, const char* id) {
        char buffer[OPENKNX_MAX_LOG_PREFIX_LENGTH] = {};
        sprintf(buffer, "%s<%s>", prefix, id);
        return std::string(buffer);
    }
    void Logger::beforeLog() {}
    ''')
    try:
        runpy.run_path(str(script), init_globals=context)
    except RuntimeError as error:
        assert 'Unsafe OGM-Common logger' in str(error) and '73bd379' in str(error)
    else:
        raise AssertionError('The crashing logger must not be accepted')
print('Common logger build guard accepts the fixed source and rejects the crashing implementation')
