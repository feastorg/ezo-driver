#!/bin/sh
# Log the DO circuit at 0x61 once a minute for 15 h, read-only, to
# do-overnight.log. Run from ~/ezo-bench with nohup so it outlives ssh.
cd "$(dirname "$0")" || exit 1
venv/bin/python -u ezo_bench.py watch 0x61 do 54000 \
  | mawk -W interactive 'NR == 1 || NR % 100 == 2 { print; fflush() }' > do-overnight.log 2>&1
