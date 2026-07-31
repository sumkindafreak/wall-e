"""Regenerate showduino_version_build.h before each PlatformIO build."""
Import("env")
import os
import subprocess

root = os.path.abspath(os.path.join(env["PROJECT_DIR"], ".."))
script = os.path.join(root, "scripts", "gen_showduino_version.sh")
if os.path.isfile(script):
    subprocess.run(["bash", script], cwd=root, check=True)
