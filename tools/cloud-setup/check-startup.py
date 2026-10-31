"""Check the actual main(), its database schema, and restart behavior."""
import sqlite3
import subprocess
import time


expected = {
    "Variants", "ReactionSteps", "LiteMetadata",
}
for attempt in range(2):
    process = subprocess.Popen(
        ["bash", "/workspace/.cloud-setup/start.sh"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    try:
        time.sleep(1)
        if process.poll() is not None:
            raise RuntimeError("Application exited before startup completed")
        with sqlite3.connect(
            "file:/workspace/.cloud-setup/run/variants-lite.db?mode=ro", uri=True
        ) as database:
            tables = {row[0] for row in database.execute(
                "SELECT name FROM sqlite_master WHERE type='table'"
            )}
            if not expected.issubset(tables):
                raise RuntimeError(f"Application schema incomplete: {tables}")
            integrity = database.execute("PRAGMA integrity_check").fetchone()[0]
            if integrity != "ok":
                raise RuntimeError(f"SQLite integrity check failed: {integrity}")
    finally:
        if process.poll() is None:
            process.terminate()
        try:
            output, _ = process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            output, _ = process.communicate()
            raise RuntimeError("Application did not terminate")
        if output.strip():
            print(output.strip())
    if process.returncode != -15:
        raise RuntimeError(f"Unexpected application status: {process.returncode}")
    if "driver not loaded" in output.lower() or "failed" in output.lower():
        raise RuntimeError(f"Application startup error: {output}")
    print(f"PASS actual application startup {attempt + 1}, schema, integrity, stop")
