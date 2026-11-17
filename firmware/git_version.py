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

# Mark builds whose firmware sources/config differ from the commit, so a
# log header never claims a commit the board is not actually running.
# If git cannot tell (e.g. a broken index), say so instead of guessing.
if commit != "unknown":
    rc = subprocess.call(
        ["git", "diff", "--quiet", "HEAD", "--", "firmware", "platformio.ini"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    if rc == 1:
        commit += "-dirty"
    elif rc != 0:
        commit += "-unverified"

env.Append(CPPDEFINES=[("FIRMWARE_GIT_COMMIT", '\\"%s\\"' % commit)])
