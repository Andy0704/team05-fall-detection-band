"""PlatformIO pre-build script: inject the current git commit hash as a
build-time macro, so the log header (CLAUDE.md code rule 5) can record
which firmware produced a given log without hand-editing anything.
"""
import subprocess

Import("env")

try:
    commit = (
        subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"], stderr=subprocess.DEVNULL
        )
        .decode()
        .strip()
    )
except Exception:
    commit = "unknown"

env.Append(CPPDEFINES=[("FIRMWARE_GIT_COMMIT", '\\"%s\\"' % commit)])
