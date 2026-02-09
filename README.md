# ArchMaster 📦

A modern, feature-rich package management dashboard for Arch Linux, built with C++17 and Qt6.

![Qt6](https://img.shields.io/badge/Qt-6-41CD52?style=flat-square&logo=qt)
![C++17](https://img.shields.io/badge/C++-17-00599C?style=flat-square&logo=cplusplus)
![Arch Linux](https://img.shields.io/badge/Arch-Linux-1793D1?style=flat-square&logo=archlinux)
![License](https://img.shields.io/badge/License-MIT-blue?style=flat-square)

---

## ✨ Features

### 📦 Package Management
- **Package Explorer** - Browse all installed packages with powerful search and filters
- **Package Details** - View version, size, install date, dependencies, and required-by info
- **Personal Notes & Tags** - Add notes and tags to remember why you installed packages
- **Keep/Review Flags** - Mark packages as "Keep" (important) or "Review" (check later)
- **Package Pinning** - Pin packages to prevent updates (adds to `IgnorePkg` in pacman.conf)
- **Version Management** - Downgrade to previous versions via Arch Linux Archive

### 🔍 Search & Discovery
- **Repository Search** - Search official repos with `pacman -Ss`
- **AUR Integration** - Search and install AUR packages via yay/paru
- **Find by File** - Search for packages that provide a specific file (`pacman -F`)

### 📊 Analytics Dashboard
- **Quick Stats** - Total, Explicit, Dependencies, Orphans, Total Size
- **Disk Usage Chart** - Pie chart showing explicit vs dependency disk usage
- **Installation Timeline** - Monthly installation trends over the last 12 months
- **Top 10 Largest Packages** - Quickly identify space hogs
- **Orphan Packages** - List packages no longer required by anything

### ⬆️ Update Manager
- **Update Check** - Scan for available updates with `checkupdates`
- **Security Flags** - Highlights security-related packages (linux, openssl, etc.)
- **Major Version Alerts** - Warns about major version changes
- **Selective Updates** - Choose which packages to update
- **Update History** - View recent package operations from pacman.log

### 📋 Package Profiles
- **7 Built-in Profiles**:
  - 🔧 Base Development (git, make, cmake, gcc, gdb, valgrind)
  - 🐍 Python Development (python, pip, virtualenv, pytest, black)
  - 🌐 Web Development (nodejs, npm, yarn, typescript, deno)
  - 🦀 Rust Development (rust, cargo, rustfmt, rust-analyzer)
  - ⚙️ C/C++ Development (gcc, clang, cmake, ninja, gdb, lldb)
  - 🐳 Container & DevOps (docker, compose, kubectl, helm, terraform)
  - 🗄️ Database Tools (postgresql, mariadb, sqlite, redis, mongodb)
- **Custom Profiles** - Create profiles from your installed packages
- **One-Click Install** - Install all packages in a profile at once
- **Export/Import** - Save profiles as JSON for backup or sharing

### ⚙️ Control Panel
- **System Update** - Run `pacman -Syu` with privilege escalation
- **Cache Cleaning** - Clean package cache with `pacman -Sc`
- **Orphan Removal** - Select and remove orphan packages
- **Secure Execution** - All privileged commands run via pkexec

### 🎨 User Experience
- **Dark Theme** - Beautiful Catppuccin-inspired dark mode
- **Light Theme** - Clean light mode alternative
- **Keyboard Shortcuts** - Ctrl+F for search, Ctrl+R to refresh
- **Data Export/Import** - Backup your notes, tags, and settings

---

## 📸 Screenshots

*Coming soon!*

---

## 🛠️ Dependencies

Install the required packages on Arch Linux:

```bash
sudo pacman -S qt6-base qt6-charts qt6-svg cmake pkgconf base-devel
```

**Optional** (for AUR support):
```bash
yay -S yay  # or paru
```

---

## 🏗️ Building

```bash
# Navigate to project directory
cd ~/manager

# Configure with CMake
cmake -B build

# Build the project
cmake --build build

# Run the application
./build/bin/archmaster
```

---

## 📥 Installation

After building, install as a system command and desktop app:

```bash
# Make launcher executable
chmod +x /home/dev/manager/archmaster

# Add to application menu (rofi/dmenu/desktop)
cp /home/dev/manager/archmaster.desktop ~/.local/share/applications/

# Create system-wide 'archmaster' command
sudo ln -sf /home/dev/manager/build/bin/archmaster /usr/local/bin/archmaster
```

Now you can:
- Run `archmaster` from any terminal
- Search "ArchMaster" in rofi or your app launcher

---

## 📁 Project Structure

```
archmaster/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── resources/                  # Icons and assets
│   └── styles.qss              # Qt stylesheet
└── src/
    ├── main.cpp                # Application entry point
    │
    ├── core/                   # Backend Services
    │   ├── PackageManager      # libalpm wrapper for package queries
    │   ├── Database            # SQLite for user data (notes, tags, flags)
    │   ├── AURClient           # AUR RPC API integration
    │   ├── PacmanConfig        # Read/write IgnorePkg in pacman.conf
    │   └── ProfileManager      # Package profile management
    │
    ├── models/                 # Data Models
    │   ├── Package             # Package data structure
    │   └── PackageListModel    # Qt model for package table
    │
    ├── ui/                     # User Interface Components
    │   ├── MainWindow          # Main application window with toolbar
    │   ├── PackageView         # Package list, details, and actions
    │   ├── SearchView          # Repo/AUR/File search interface
    │   ├── AnalyticsView       # Charts and statistics dashboard
    │   ├── ControlPanel        # System operations panel
    │   ├── UpdateManager       # Update checking and installation
    │   ├── ProfileView         # Package profiles management
    │   ├── PrivilegedRunner    # Secure sudo command execution
    │   ├── ChartPopup          # Expanded chart view
    │   └── LoadingOverlay      # Loading indicator
    │
    └── utils/                  # Utilities
        └── Config              # Application settings persistence
```

---

## 🎯 Usage Guide

### Package View (📦 Packages)
| Action | Description |
|--------|-------------|
| Search | Type in search bar to filter packages |
| Filter | Use dropdown: All, Explicit, Dependencies, Orphans, Keep, Review, Large |
| Notes | Click a package, add notes in the right panel |
| Tags | Add custom tags like "essential", "gaming", "work" |
| Pin | Click "📌 Pin" to prevent a package from updating |
| Version | Click "🔄 Change Version" to downgrade via Arch Archive |

### Search View (🔍 Search)
| Source | Description |
|--------|-------------|
| Repository | Search official Arch repos with `pacman -Ss` |
| AUR | Search Arch User Repository (requires yay/paru) |
| Find by File | Find which package provides a file (`pacman -F`) |

### Analytics View (📊 Analytics)
- View disk usage breakdown between explicit and dependency packages
- See your installation timeline over the past year
- Identify the largest packages consuming disk space
- Find and clean up orphan packages

### Updates View (⬆️ Updates)
- Click "🔄 Check for Updates" to scan
- 🔒 indicates security-related packages
- ⚠️ indicates major version changes
- Use checkboxes to select specific packages
- Click "📜 History" to view recent package operations

### Profiles View (📋 Profiles)
- Select a built-in or custom profile
- ✅ means package is installed, ⬇️ means missing
- Click "⬇️ Install All Packages" to install missing ones
- Click "➕ Create from System" to save your installed packages

---

## 💾 Data Storage

User data is stored in SQLite:
```
~/.local/share/ArchMaster/archmaster.db
```

Contains:
- Personal notes for packages
- Custom tags
- Keep/Review flags
- Application settings

Custom profiles are stored in:
```
~/.local/share/ArchMaster/profiles.json
```

---

## ⌨️ Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+F` | Focus search bar |
| `Ctrl+R` | Refresh packages |
| `Ctrl+1` | Switch to Packages view |
| `Ctrl+2` | Switch to Analytics view |
| `Ctrl+3` | Switch to Control Panel |
| `Ctrl+4` | Switch to Search view |
| `Ctrl+5` | Switch to Updates view |

---

## 🔐 Security

- **No saved passwords** - Password is never stored
- **pkexec for privileges** - Standard polkit authentication
- **Direct commands** - Commands are executed directly, not through shell

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

---

## 📄 License

MIT License - See [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgments

- [Qt Project](https://www.qt.io/) - UI framework
- [libalpm](https://archlinux.org/pacman/libalpm.5.html) - Package management library
- [Catppuccin](https://github.com/catppuccin/catppuccin) - Color palette inspiration
- [Arch Linux](https://archlinux.org/) - The best distro 🐧
