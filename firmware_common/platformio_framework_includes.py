"""PlatformIO pre-script: expose Arduino framework library include dirs to all units."""
Import("env")
import os

_common_include = os.path.abspath(
    os.path.join(env["PROJECT_DIR"], "..", "firmware_common", "include")
)
if os.path.isdir(_common_include):
    env.Append(CPPPATH=[_common_include])

framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
if framework_dir:
    for lib in ("WiFi", "Network", "WebServer", "FS", "NetworkClientSecure"):
        inc = os.path.join(framework_dir, "libraries", lib, "src")
        if os.path.isdir(inc):
            env.Append(CPPPATH=[inc])

env.Append(CCFLAGS=["-DLV_USE_DRAW_SW_ASM=LV_DRAW_SW_ASM_NONE"])


def _skip_lvgl_helium_asm(node):
    path = str(node).replace("\\", "/")
    if ".pio/libdeps" in path and "/lvgl/" in path and path.endswith(".S"):
        return None
    return node


env.AddBuildMiddleware(_skip_lvgl_helium_asm)
