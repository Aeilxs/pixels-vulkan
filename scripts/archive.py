#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import platform
import subprocess
import sys
import zipfile
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence


SCHEMA_VERSION = 2
RECENT_COMMIT_COUNT = 12

EXCLUDED_DIRS = {
    ".git",
    ".cache",
    ".idea",
    "archives",
    "build",
    "vendor",
}

REVIEW_DIRS = {
    "cmake",
    "docs",
    "include",
    "scripts",
    "shaders",
    "src",
    "test",
    "tests",
}

SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".inl",
    ".ipp",
    ".tpp",
}

SHADER_SUFFIXES = {
    ".comp",
    ".frag",
    ".geom",
    ".glsl",
    ".mesh",
    ".task",
    ".tesc",
    ".tese",
    ".vert",
}

TEXT_SUFFIXES = SOURCE_SUFFIXES | SHADER_SUFFIXES | {
    ".bat",
    ".cmake",
    ".json",
    ".md",
    ".ps1",
    ".py",
    ".sh",
    ".toml",
    ".txt",
    ".yaml",
    ".yml",
}

ROOT_FILES = {
    ".clang-format",
    ".clang-tidy",
    ".editorconfig",
    ".gitignore",
    "CMakeLists.txt",
    "CMakePresets.json",
    "LICENSE",
    "LICENSE.md",
    "README.md",
    "conanfile.py",
    "conanfile.txt",
    "vcpkg.json",
}


class ArchiveError(RuntimeError):
    pass


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create a compact source snapshot for code review."
    )
    parser.add_argument(
        "--include-assets",
        action="store_true",
        help="Include assets/ in the zip. By default only an asset inventory is recorded.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be archived without writing a zip.",
    )
    return parser.parse_args()


def project_root() -> Path:
    script = Path(__file__).resolve()
    return script.parent.parent if script.parent.name == "scripts" else script.parent


def run(command: Sequence[str], cwd: Path) -> Optional[str]:
    try:
        result = subprocess.run(
            list(command),
            cwd=str(cwd),
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except FileNotFoundError:
        return None

    if result.returncode != 0:
        return None

    return result.stdout.strip()


def git(project: Path, *args: str) -> Optional[str]:
    return run(["git", *args], project)


def first_line(command: Sequence[str], project: Path) -> Optional[str]:
    output = run(command, project)
    return output.splitlines()[0] if output else None


def git_repo_files(project: Path) -> Optional[List[str]]:
    try:
        result = subprocess.run(
            ["git", "ls-files", "-co", "--exclude-standard", "-z"],
            cwd=str(project),
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
    except FileNotFoundError:
        return None

    if result.returncode != 0:
        return None

    output = result.stdout.decode("utf-8", errors="surrogateescape")
    return [path for path in output.split("\0") if path]


def fallback_files(project: Path) -> List[str]:
    files: List[str] = []

    for root, directories, filenames in os.walk(project):
        directories[:] = [
            directory
            for directory in directories
            if directory not in EXCLUDED_DIRS and directory != ".vscode"
        ]

        root_path = Path(root)
        for filename in filenames:
            files.append((root_path / filename).relative_to(project).as_posix())

    return files


def should_include(relative: Path, include_assets: bool) -> bool:
    if not relative.parts or relative.is_absolute() or ".." in relative.parts:
        return False

    if any(part in EXCLUDED_DIRS for part in relative.parts):
        return False

    top = relative.parts[0]
    suffix = relative.suffix.lower()

    if top == "assets":
        return include_assets

    # Never miss source just because it moved to a new folder.
    if suffix in SOURCE_SUFFIXES or suffix in SHADER_SUFFIXES:
        return True

    if len(relative.parts) == 1:
        if relative.name in ROOT_FILES:
            return True
        return suffix in {".md", ".txt"}

    if top not in REVIEW_DIRS:
        return False

    return relative.name == "CMakeLists.txt" or suffix in TEXT_SUFFIXES


def collect_files(project: Path, include_assets: bool) -> List[Path]:
    candidates = git_repo_files(project)
    if candidates is None:
        candidates = fallback_files(project)

    files: Dict[str, Path] = {}

    for raw_path in candidates:
        relative = Path(raw_path)
        full_path = project / relative

        if not should_include(relative, include_assets):
            continue
        if not full_path.is_file() or full_path.is_symlink():
            continue

        files[relative.as_posix()] = relative

    return [files[key] for key in sorted(files)]


def line_count(path: Path) -> int:
    data = path.read_bytes()
    if not data:
        return 0
    return data.count(b"\n") + (0 if data.endswith(b"\n") else 1)


def source_index(project: Path, files: Sequence[Path]) -> Dict[str, Any]:
    entries: List[Dict[str, Any]] = []
    by_extension: Dict[str, Dict[str, int]] = defaultdict(
        lambda: {"files": 0, "lines": 0, "bytes": 0}
    )

    source_files = 0
    source_lines = 0
    shader_files = 0
    shader_lines = 0

    for relative in files:
        suffix = relative.suffix.lower()
        if suffix not in SOURCE_SUFFIXES and suffix not in SHADER_SUFFIXES:
            continue

        full_path = project / relative
        lines = line_count(full_path)
        size = full_path.stat().st_size
        kind = "shader" if suffix in SHADER_SUFFIXES else "source"

        entries.append(
            {
                "path": relative.as_posix(),
                "kind": kind,
                "lines": lines,
                "bytes": size,
            }
        )

        by_extension[suffix]["files"] += 1
        by_extension[suffix]["lines"] += lines
        by_extension[suffix]["bytes"] += size

        if kind == "source":
            source_files += 1
            source_lines += lines
        else:
            shader_files += 1
            shader_lines += lines

    return {
        "summary": {
            "source_files": source_files,
            "source_lines": source_lines,
            "shader_files": shader_files,
            "shader_lines": shader_lines,
            "by_extension": dict(sorted(by_extension.items())),
        },
        "files": entries,
    }


def inventory(project: Path, directory_name: str) -> Dict[str, Any]:
    directory = project / directory_name
    if not directory.is_dir():
        return {"present": False, "file_count": 0, "bytes": 0, "files": []}

    files: List[Dict[str, Any]] = []
    total_bytes = 0

    for path in sorted(directory.rglob("*")):
        if not path.is_file() or path.is_symlink():
            continue

        size = path.stat().st_size
        total_bytes += size
        files.append(
            {
                "path": path.relative_to(project).as_posix(),
                "bytes": size,
            }
        )

    return {
        "present": True,
        "file_count": len(files),
        "bytes": total_bytes,
        "files": files[:200],
        "files_truncated": len(files) > 200,
    }


def recent_commits(project: Path) -> List[Dict[str, str]]:
    output = git(
        project,
        "log",
        f"-{RECENT_COMMIT_COUNT}",
        "--pretty=format:%H%x1f%h%x1f%cI%x1f%s",
    )
    if not output:
        return []

    commits: List[Dict[str, str]] = []
    for line in output.splitlines():
        parts = line.split("\x1f", 3)
        if len(parts) != 4:
            continue
        full_hash, short_hash, committed_at, subject = parts
        commits.append(
            {
                "commit": full_hash,
                "short_commit": short_hash,
                "committed_at": committed_at,
                "subject": subject,
            }
        )
    return commits


def ahead_behind(project: Path, reference: str) -> Optional[Dict[str, int]]:
    exists = git(project, "rev-parse", "--verify", "--quiet", reference)
    if not exists:
        return None

    output = git(project, "rev-list", "--left-right", "--count", f"{reference}...HEAD")
    if not output:
        return None

    parts = output.split()
    if len(parts) != 2:
        return None

    return {"behind": int(parts[0]), "ahead": int(parts[1])}


def git_snapshot(project: Path) -> Dict[str, Any]:
    if git(project, "rev-parse", "--is-inside-work-tree") != "true":
        return {"available": False}

    branch = git(project, "branch", "--show-current") or "DETACHED"
    status = git(project, "status", "--porcelain=v1", "--untracked-files=all") or ""
    upstream = git(
        project,
        "rev-parse",
        "--abbrev-ref",
        "--symbolic-full-name",
        "@{upstream}",
    )

    refs: Dict[str, Dict[str, int]] = {}
    for reference in ("main", "develop", "origin/main", "origin/develop"):
        relation = ahead_behind(project, reference)
        if relation is not None:
            refs[reference] = relation

    return {
        "available": True,
        "branch": branch,
        "commit": git(project, "rev-parse", "HEAD"),
        "short_commit": git(project, "rev-parse", "--short=12", "HEAD"),
        "describe": git(project, "describe", "--tags", "--always", "--dirty"),
        "is_dirty": bool(status),
        "status_porcelain": status.splitlines(),
        "upstream": upstream,
        "relative_to_common_branches": refs,
        "recent_commits": recent_commits(project),
    }


def git_diff(project: Path, staged: bool) -> str:
    command = ["git", "diff", "--no-ext-diff", "--no-color"]
    if staged:
        command.append("--cached")

    command.extend(
        [
            "--",
            ".",
            ":(exclude)vendor/**",
            ":(exclude)assets/**",
            ":(exclude)archives/**",
            ":(exclude)build/**",
        ]
    )

    return run(command, project) or ""


def environment(project: Path) -> Dict[str, Optional[str]]:
    return {
        "operating_system": platform.platform(),
        "machine": platform.machine(),
        "python": platform.python_version(),
        "git": first_line(["git", "--version"], project),
        "cmake": first_line(["cmake", "--version"], project),
        "ninja": first_line(["ninja", "--version"], project),
        "compiler": first_line(["c++", "--version"], project),
        "glslangValidator": first_line(["glslangValidator", "--version"], project),
    }


def human_size(size: int) -> str:
    value = float(size)
    for unit in ("B", "KiB", "MiB", "GiB"):
        if value < 1024.0 or unit == "GiB":
            return f"{int(value)} {unit}" if unit == "B" else f"{value:.1f} {unit}"
        value /= 1024.0
    return f"{size} B"


def review_context(manifest: Dict[str, Any]) -> str:
    git_info = manifest["git"]
    summary = manifest["source"]["summary"]
    vendor = manifest["omitted"]["vendor"]
    assets = manifest["omitted"]["assets"]

    lines = [
        "# Code review snapshot",
        "",
        f"- Project: `{manifest['project']['name']}`",
        f"- Created: `{manifest['archive']['created_at_utc']}`",
        f"- Included: **{manifest['archive']['included_file_count']} files** "
        f"({human_size(manifest['archive']['included_bytes'])})",
        f"- C/C++: **{summary['source_files']} files / {summary['source_lines']} lines**",
        f"- Shaders: **{summary['shader_files']} files / {summary['shader_lines']} lines**",
        "",
        "## Git",
        "",
    ]

    if git_info.get("available"):
        lines.extend(
            [
                f"- Branch: `{git_info['branch']}`",
                f"- Commit: `{git_info.get('short_commit') or 'unknown'}`",
                f"- Describe: `{git_info.get('describe') or 'n/a'}`",
                f"- Dirty: **{'yes' if git_info['is_dirty'] else 'no'}**",
            ]
        )
        if git_info.get("upstream"):
            lines.append(f"- Upstream: `{git_info['upstream']}`")

        refs = git_info.get("relative_to_common_branches", {})
        if refs:
            lines.append("- Relative to common branches:")
            for name, relation in refs.items():
                lines.append(
                    f"  - `{name}`: ahead {relation['ahead']}, behind {relation['behind']}"
                )

        lines.extend(["", "### Working tree", ""])
        status = git_info.get("status_porcelain", [])
        lines.extend(f"- `{line}`" for line in status)
        if not status:
            lines.append("Clean working tree.")

        lines.extend(["", "### Recent commits", ""])
        for commit in git_info.get("recent_commits", []):
            lines.append(f"- `{commit['short_commit']}` {commit['subject']}")
    else:
        lines.append("Git metadata unavailable.")

    lines.extend(
        [
            "",
            "## Omitted payloads",
            "",
            f"- `vendor/`: {vendor['file_count']} files, {human_size(vendor['bytes'])}",
            f"- `assets/`: {assets['file_count']} files, {human_size(assets['bytes'])}",
            "",
            "`vendor/` is always excluded. `assets/` is excluded by default; use "
            "`--include-assets` when the actual images are useful for the review.",
            "",
            "## Included review context",
            "",
            "- `.archive-manifest.json`: complete machine-readable snapshot.",
            "- `.archive-context/source-index.json`: every C/C++ and shader file with line/byte counts.",
            "- `.archive-context/files.txt`: exact archive payload.",
            "- `.archive-context/git-status.txt`: branch and working-tree state.",
            "- `.archive-context/git-log.txt`: recent history.",
            "- `.archive-context/git-diff.patch`: unstaged diff, excluding vendor/assets/build/archives.",
            "- `.archive-context/git-diff-staged.patch`: staged diff with the same exclusions.",
            "",
        ]
    )

    return "\n".join(lines)


def add_text(archive: zipfile.ZipFile, root: str, name: str, text: str) -> None:
    archive.writestr(
        f"{root}/{name}",
        text.encode("utf-8"),
        compress_type=zipfile.ZIP_DEFLATED,
    )


def main() -> int:
    args = parse_args()

    try:
        project = project_root()
        project_name = project.name
        archive_dir = project / "archives"

        now = datetime.now(timezone.utc)
        timestamp = now.astimezone().strftime("%Y%m%d-%H%M%S")
        archive_name = f"{project_name}-{timestamp}.zip"
        archive_path = archive_dir / archive_name

        files = collect_files(project, args.include_assets)
        if not files:
            raise ArchiveError("No reviewable files found")

        included_bytes = sum((project / path).stat().st_size for path in files)
        source = source_index(project, files)
        git_info = git_snapshot(project)
        vendor = inventory(project, "vendor")
        assets = inventory(project, "assets")

        manifest: Dict[str, Any] = {
            "schema_version": SCHEMA_VERSION,
            "archive": {
                "name": archive_name,
                "created_at_utc": now.strftime("%Y-%m-%dT%H:%M:%SZ"),
                "purpose": "code_review_snapshot",
                "included_file_count": len(files),
                "included_bytes": included_bytes,
                "assets_included": bool(args.include_assets),
            },
            "project": {"name": project_name},
            "source": source,
            "git": git_info,
            "environment": environment(project),
            "omitted": {
                "vendor": vendor,
                "assets": assets,
            },
            "policy": {
                "vendor_included": False,
                "ignored_files_included": False,
                "symlinks_included": False,
            },
        }

        if args.dry_run:
            print(f"Would create: {archive_path}")
            print(f"Included: {len(files)} files ({human_size(included_bytes)})")
            print(
                f"C/C++: {source['summary']['source_files']} files / "
                f"{source['summary']['source_lines']} lines"
            )
            print(
                f"Shaders: {source['summary']['shader_files']} files / "
                f"{source['summary']['shader_lines']} lines"
            )
            print(f"Vendor omitted: {vendor['file_count']} files / {human_size(vendor['bytes'])}")
            print(
                f"Assets {'included' if args.include_assets else 'omitted'}: "
                f"{assets['file_count']} files / {human_size(assets['bytes'])}"
            )
            return 0

        archive_dir.mkdir(parents=True, exist_ok=True)
        if archive_path.exists():
            raise ArchiveError(f"Refusing to overwrite: {archive_path}")

        status = git(project, "status", "--short", "--branch", "--untracked-files=all") or "Git metadata unavailable."
        log = git(
            project,
            "log",
            f"-{RECENT_COMMIT_COUNT}",
            "--date=iso-strict",
            "--pretty=format:%h %cI %s",
        ) or "Git history unavailable."

        unstaged_diff = git_diff(project, staged=False) if git_info.get("available") else ""
        staged_diff = git_diff(project, staged=True) if git_info.get("available") else ""

        with zipfile.ZipFile(
            archive_path,
            mode="x",
            compression=zipfile.ZIP_DEFLATED,
            compresslevel=9,
            allowZip64=True,
        ) as archive:
            for relative in files:
                archive.write(project / relative, f"{project_name}/{relative.as_posix()}")

            add_text(
                archive,
                project_name,
                ".archive-manifest.json",
                json.dumps(manifest, indent=2, ensure_ascii=False) + "\n",
            )
            add_text(archive, project_name, "REVIEW_CONTEXT.md", review_context(manifest))
            add_text(
                archive,
                project_name,
                ".archive-context/source-index.json",
                json.dumps(source, indent=2, ensure_ascii=False) + "\n",
            )
            add_text(
                archive,
                project_name,
                ".archive-context/files.txt",
                "\n".join(path.as_posix() for path in files) + "\n",
            )
            add_text(archive, project_name, ".archive-context/git-status.txt", status + "\n")
            add_text(archive, project_name, ".archive-context/git-log.txt", log + "\n")
            add_text(
                archive,
                project_name,
                ".archive-context/git-diff.patch",
                unstaged_diff + ("\n" if unstaged_diff else ""),
            )
            add_text(
                archive,
                project_name,
                ".archive-context/git-diff-staged.patch",
                staged_diff + ("\n" if staged_diff else ""),
            )

        print("Archive created:")
        print(archive_path)
        print()
        print(f"Included: {len(files)} files ({human_size(included_bytes)})")
        print(
            f"Sources:  {source['summary']['source_files']} C/C++ files / "
            f"{source['summary']['source_lines']} lines"
        )
        print(f"Vendor:   omitted ({human_size(vendor['bytes'])})")
        if args.include_assets:
            print(f"Assets:   included ({human_size(assets['bytes'])})")
        else:
            print(f"Assets:   omitted ({human_size(assets['bytes'])}); use --include-assets if needed")
        print()
        print(f"Review entrypoint: {project_name}/REVIEW_CONTEXT.md")
        return 0

    except (ArchiveError, OSError, zipfile.BadZipFile) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
