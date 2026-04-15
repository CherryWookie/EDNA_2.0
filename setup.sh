#!/bin/zsh

SCRIPT_PATH="${(%):-%N}"
PROJECT_DIR="$(cd "$(dirname "$SCRIPT_PATH")" && pwd)"
cd "$PROJECT_DIR"

# pyenv setup
export PYENV_ROOT="$HOME/.pyenv"
export PATH="$PYENV_ROOT/shims:$PYENV_ROOT/bin:$PATH"

if command -v pyenv >/dev/null 2>&1; then
  eval "$(pyenv init --path)"
  eval "$(pyenv init - zsh)"
fi

pyenv shell 3.12.9

# Load ESP-IDF v6
cd ~/esp/esp-idf-v6.0 || exit 1
. ./export.sh

# Go to your project
cd "$PROJECT_DIR" || exit 1
echo
echo "======================"
echo "Python: $(python3 --version)"
echo "IDF_PATH: $IDF_PATH"
echo "Project: $(pwd)"
echo "ESP-IDF environment ready."
echo "Ready in $PROJECT_DIR"
echo "======================"
echo
