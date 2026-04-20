"""
Pre-build script to patch LilyGoLib library.json with missing dependencies.
LilyGoLib doesn't declare TinyGPSPlus, NFC-RFAL-fork, ST25R3916-fork as
dependencies, causing PlatformIO LDF to fail resolving them.
"""
import os
import json

Import("env")

libdeps = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"))
lilygo_json = os.path.join(libdeps, "LilyGoLib", "library.json")

if os.path.isfile(lilygo_json):
    with open(lilygo_json) as f:
        data = json.load(f)

    deps = data.get("dependencies", [])
    names = [d.get("name", "") for d in deps]

    missing = ["TinyGPSPlus", "NFC-RFAL-fork", "ST25R3916-fork", "ESP8266Audio"]
    changed = False
    for lib in missing:
        if lib not in names:
            deps.append({"name": lib, "version": "*"})
            changed = True

    if changed:
        data["dependencies"] = deps
        with open(lilygo_json, "w") as f:
            json.dump(data, f, indent=4)
        print("[extra_script] Patched LilyGoLib library.json with missing dependencies")
