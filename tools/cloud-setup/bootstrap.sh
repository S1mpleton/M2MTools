#!/bin/bash
set -euo pipefail
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
repo_dir="$(git -C "$script_dir" rev-parse --show-toplevel)"
if [[ "$repo_dir" != /workspace/M2MTools ]]; then
  printf '%s\n' 'This configuration expects the checkout at /workspace/M2MTools.' >&2
  exit 1
fi

# Install only the small setup inputs. Dependencies and builds stay outside Git.
# Preserve existing runtime files; report differing content instead of replacing it.
for relative_file in apt.conf debian.sources env.sh install.sh start.sh \
    check-startup.py include/core/Result.h smoke/CMakeLists.txt smoke/smoke.cpp; do
  source_path="$script_dir/$relative_file"
  destination="/workspace/.cloud-setup/$relative_file"
  if [[ -L "$destination" ]]; then
    printf 'Refusing to replace symlink: %s\n' "$destination" >&2
    exit 1
  elif [[ -e "$destination" ]]; then
    if ! cmp -s "$source_path" "$destination"; then
      printf 'Existing setup file differs; reconcile it first: %s\n' "$destination" >&2
      exit 1
    fi
  else
    install -D -m 600 "$source_path" "$destination"
  fi
done

bash /workspace/.cloud-setup/install.sh
python3 /workspace/.cloud-setup/check-startup.py
