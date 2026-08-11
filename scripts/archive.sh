#!/usr/bin/env bash

set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
readonly PROJECT_NAME="$(basename -- "${PROJECT_DIR}")"

readonly TIMESTAMP="$(date '+%Y%m%d-%H%M%S')"
readonly CREATED_AT_UTC="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"

readonly ARCHIVE_DIR="${PROJECT_DIR}/archives"
readonly ARCHIVE_NAME="${PROJECT_NAME}-${TIMESTAMP}.zip"
readonly ARCHIVE_PATH="${ARCHIVE_DIR}/${ARCHIVE_NAME}"

# Le manifest est placé temporairement dans le projet afin d'être inclus
# à la racine de l'archive.
readonly MANIFEST_NAME=".archive-manifest.json"
readonly MANIFEST_PATH="${PROJECT_DIR}/${MANIFEST_NAME}"

cleanup() {
    rm -f -- "${MANIFEST_PATH}"
}

trap cleanup EXIT

mkdir -p -- "${ARCHIVE_DIR}"

if [[ -e "${MANIFEST_PATH}" ]]; then
    echo "Refusing to overwrite existing file: ${MANIFEST_PATH}" >&2
    exit 1
fi

export PROJECT_DIR
export PROJECT_NAME
export ARCHIVE_NAME
export CREATED_AT_UTC
export MANIFEST_PATH

# Python est utilisé uniquement pour produire un JSON valide sans avoir
# à gérer manuellement l'échappement des branches, commits et chemins.
python3 <<'PYTHON'
import json
import os
import platform
import subprocess
from pathlib import Path
from typing import Optional


project_dir = Path(os.environ["PROJECT_DIR"])
manifest_path = Path(os.environ["MANIFEST_PATH"])


def run_command(command: list[str]) -> Optional[str]:
    try:
        result = subprocess.run(
            command,
            cwd=project_dir,
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


def first_line(command: list[str]) -> Optional[str]:
    output = run_command(command)

    if not output:
        return None

    return output.splitlines()[0]


branch = run_command(["git", "branch", "--show-current"])

if not branch:
    branch = "DETACHED"

commit = run_command(["git", "rev-parse", "HEAD"])
short_commit = run_command(["git", "rev-parse", "--short=12", "HEAD"])
describe = run_command(["git", "describe", "--tags", "--always", "--dirty"])

status_output = run_command(
    [
        "git",
        "status",
        "--porcelain=v1",
        "--untracked-files=all",
    ]
)

status_lines = status_output.splitlines() if status_output else []

tracked_files_output = run_command(["git", "ls-files"])
tracked_files = tracked_files_output.splitlines() if tracked_files_output else []

recent_commits_output = run_command(
    [
        "git",
        "log",
        "-5",
        "--pretty=format:%H%x1f%h%x1f%cI%x1f%s",
    ]
)

recent_commits: list[dict[str, str]] = []

if recent_commits_output:
    for line in recent_commits_output.splitlines():
        parts = line.split("\x1f", 3)

        if len(parts) != 4:
            continue

        full_hash, short_hash, committed_at, subject = parts

        recent_commits.append(
            {
                "commit": full_hash,
                "short_commit": short_hash,
                "committed_at": committed_at,
                "subject": subject,
            }
        )

key_files = [
    "CMakeLists.txt",
    "src/config.hpp",
    "src/main.cpp",
    "src/platform/sdl_context.hpp",
    "src/platform/window.hpp",
    "src/renderer/vulkan/instance.hpp",
    "src/renderer/vulkan/surface.hpp",
    "src/renderer/vulkan/physical_device.hpp",
    "src/renderer/vulkan/queue_family_indices.hpp",
]

manifest = {
    "schema_version": 1,
    "archive": {
        "name": os.environ["ARCHIVE_NAME"],
        "created_at_utc": os.environ["CREATED_AT_UTC"],
        "manifest_path": os.environ["MANIFEST_PATH"].replace(
            str(project_dir) + "/",
            "",
        ),
    },
    "project": {
        "name": os.environ["PROJECT_NAME"],
        "tracked_file_count": len(tracked_files),
        "key_files_present": [
            path
            for path in key_files
            if (project_dir / path).is_file()
        ],
    },
    "git": {
        "branch": branch,
        "commit": commit,
        "short_commit": short_commit,
        "describe": describe,
        "is_dirty": bool(status_lines),
        "uncommitted_change_count": len(status_lines),
        "status_porcelain": status_lines,
        "recent_commits": recent_commits,
    },
    "environment": {
        "operating_system": platform.platform(),
        "machine": platform.machine(),
        "python": platform.python_version(),
        "cmake": first_line(["cmake", "--version"]),
        "ninja": first_line(["ninja", "--version"]),
        "compiler": first_line(["c++", "--version"]),
    },
}

manifest_path.write_text(
    json.dumps(
        manifest,
        indent=2,
        ensure_ascii=False,
    )
    + "\n",
    encoding="utf-8",
)
PYTHON

cd -- "$(dirname -- "${PROJECT_DIR}")"

zip -r --quiet "${ARCHIVE_PATH}" "${PROJECT_NAME}" \
    -x "${PROJECT_NAME}/.git/*" \
    -x "${PROJECT_NAME}/build/*" \
    -x "${PROJECT_NAME}/archives/*" \
    -x "${PROJECT_NAME}/.cache/*" \
    -x "${PROJECT_NAME}/.DS_Store" \
    -x "${PROJECT_NAME}/*/.DS_Store" \
    -x "${PROJECT_NAME}/*/*/.DS_Store"

echo "Archive created:"
echo "${ARCHIVE_PATH}"
echo
echo "Manifest included as:"
echo "${PROJECT_NAME}/${MANIFEST_NAME}"
