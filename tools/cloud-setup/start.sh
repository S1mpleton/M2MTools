#!/bin/bash
set -euo pipefail
source /workspace/.cloud-setup/env.sh
mkdir -p /workspace/.cloud-setup/run /workspace/.cloud-setup/cache
# variants-lite.db is relative to the working directory in the upstream main().
cd /workspace/.cloud-setup/run
exec /workspace/.cloud-setup/build/M2MTools
