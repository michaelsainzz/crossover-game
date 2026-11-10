"""Check repository documentation and Unreal package tracking rules."""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DOCS = (
    "README.md",
    "AGENTS.md",
    "docs/SLICE.md",
    "docs/HARNESS.md",
    "docs/TOOLCHAIN.md",
    "docs/ARCHITECTURE.md",
    "docs/STATUS.md",
)
MARKDOWN_LINK = re.compile(r"\]\(([^)]+)\)")


def main():
    errors = []
    for name in DOCS:
        document = ROOT / name
        if not document.is_file():
            errors.append(f"Missing {name}")
            continue
        for target in MARKDOWN_LINK.findall(document.read_text(encoding="utf-8")):
            path = target.split("#", 1)[0]
            if path and "://" not in path and not (document.parent / path).is_file():
                errors.append(f"{name}: missing link target {path}")

    attributes = ROOT / ".gitattributes"
    lines = attributes.read_text(encoding="utf-8").splitlines() if attributes.is_file() else []
    for extension in ("uasset", "umap"):
        rule = f"*.{extension} filter=lfs diff=lfs merge=lfs -text"
        if rule not in lines:
            errors.append(f"Missing Git LFS rule for .{extension}")

    if errors:
        print("\n".join(errors))
        return 1
    print("Repository documentation and Unreal package rules passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
