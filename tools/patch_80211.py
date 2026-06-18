from os import remove, rename
from os.path import isfile, join
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    Import: Any = None
    env: Any = {}


Import("env")  # type: ignore

FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-arduinoespressif32-libs")
board_mcu = env.BoardConfig()
mcu = board_mcu.get("build.mcu", "")
patchflag_path = join(FRAMEWORK_DIR,mcu, "lib", ".patched")

# patch file only if we didn't do it befored
if not isfile(join(FRAMEWORK_DIR,mcu, "lib", ".patched")):
    original_file = join(FRAMEWORK_DIR,mcu, "lib", "libnet80211.a")
    patched_file = join(
        FRAMEWORK_DIR, mcu, "lib", "libnet80211.a.patched"
    )

    if mcu=="esp32c5" or mcu=="esp32c6" :
        env.Execute(
            "pio pkg exec -p toolchain-riscv32-esp -- riscv32-esp-elf-objcopy  --weaken-symbol=ieee80211_raw_frame_sanity_check %s %s"
            % (original_file, patched_file)
        )
    elif mcu=="esp32p4":
        """Do nothing"""
    else:
        env.Execute(
            "pio pkg exec -p toolchain-xtensa-%s -- xtensa-%s-elf-objcopy  --weaken-symbol=ieee80211_raw_frame_sanity_check %s %s"
            % (mcu, mcu, original_file, patched_file)
        )

    if isfile("%s.old" % (original_file)):
        remove("%s.old" % (original_file))

    if isfile(original_file):
        rename(original_file, "%s.old" % (original_file))
    else:
        print("Patch: Original file not found")

    if isfile(patched_file):
        rename(patched_file, original_file)
    else:
        print("Patch: Patched file not found")


    def _touch(path):
        with open(path, "w") as fp:
            fp.write("")

    env.Execute(lambda *args, **kwargs: _touch(patchflag_path))
