#!/usr/bin/env python3
"""Ad-hoc sign and verify a self-contained Apple Silicon development bundle."""

import argparse
from pathlib import Path
import subprocess


def run(*args):
    result = subprocess.run(args, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(f"command failed: {args}\n{result.stdout}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("app", type=Path)
    parser.add_argument("--sign", action="store_true", help="apply local ad-hoc signatures")
    args = parser.parse_args()
    app = args.app.resolve()
    executable = app / "Contents/MacOS/klatexformula"
    if not executable.is_file():
        parser.error("expected a built klatexformula.app bundle")

    # Include plug-ins in Contents/plugins: codesign --deep alone skips these
    # nonstandard code locations, leaving signatures invalid after fixup_bundle.
    magic = {bytes.fromhex(s) for s in (
        "feedface", "cefaedfe", "feedfacf", "cffaedfe",
        "cafebabe", "bebafeca", "cafebabf", "bfbafeca",
    )}
    binaries = []
    for path in sorted(app.rglob("*")):
        if path.is_symlink() or not path.is_file():
            continue
        with path.open("rb") as stream:
            if stream.read(4) in magic:
                binaries.append(path)
    if executable not in binaries:
        raise RuntimeError("main executable is not Mach-O")

    if args.sign:
        for path in binaries:
            run("/usr/bin/codesign", "--force", "--sign", "-", str(path))
        for framework in sorted((app / "Contents/Frameworks").glob("*.framework")):
            run("/usr/bin/codesign", "--force", "--sign", "-", str(framework))
        run("/usr/bin/codesign", "--force", "--sign", "-", str(app))

    for path in binaries:
        run("/usr/bin/lipo", str(path), "-verify_arch", "arm64")
        run("/usr/bin/codesign", "--verify", "--strict", str(path))
        identities = {line.strip() for line in
                      run("/usr/bin/otool", "-arch", "arm64", "-D", str(path)).splitlines()[1:]}
        for line in run("/usr/bin/otool", "-arch", "arm64", "-L", str(path)).splitlines()[1:]:
            dependency = line.strip().split(" (compatibility version", 1)[0]
            if dependency in identities:  # LC_ID_DYLIB is not a dependency
                continue
            if dependency.startswith(("/System/Library/", "/usr/lib/")):
                continue
            resolved = dependency.replace("@executable_path", str(executable.parent))
            resolved = resolved.replace("@loader_path", str(path.parent))
            target = Path(resolved).resolve()
            if app not in target.parents or not target.is_file():
                raise RuntimeError(f"unbundled dependency in {path}: {dependency}")
    run("/usr/bin/codesign", "--verify", "--deep", "--strict", str(app))
    print(f"Verified {len(binaries)} ARM64 Mach-O files, signatures and bundled dependencies.")


if __name__ == "__main__":
    main()
