#!/bin/bash

# Exit on error
set -e

echo "Setting up ESP-IDF environment on macOS..."

# Check for Xcode command line tools
if ! xcode-select -p &> /dev/null; then
    echo "Installing Xcode command line tools..."
    xcode-select --install
fi

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo "Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# Install required packages
echo "Installing required packages..."
brew install cmake ninja dfu-util python

# Download ESP-IDF
echo "Downloading ESP-IDF..."
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git

# Set up tools
echo "Setting up ESP-IDF tools..."
cd ~/esp/esp-idf
./install.sh

# Set up environment variables
echo "Setting up environment variables..."
echo '. $HOME/esp/esp-idf/export.sh' >> ~/.zshrc

# Activate environment
echo "Activating environment..."
source ~/.zshrc

# Verify installation
echo "Verifying installation..."
idf.py --version

echo "ESP-IDF setup complete!"
