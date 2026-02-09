cmake_minimum_required(VERSION 3.20)
project(archmaster VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Widgets 
    Sql 
    Network 
    Charts
    Svg
)

# Find system libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALPM REQUIRED libalpm)
pkg_check_modules(CURL REQUIRED libcurl)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${ALPM_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/core/PackageManager.cpp
    src/core/AURClient.cpp
    src/core/Database.cpp
    src/core/PacmanConfig.cpp
    src/core/ProfileManager.cpp
    src/models/Package.cpp
    src/models/PackageListModel.cpp
    src/ui/MainWindow.cpp
    src/ui/PackageView.cpp
    src/ui/AnalyticsView.cpp
    src/ui/ControlPanel.cpp
    src/ui/LoadingOverlay.cpp
    src/ui/ChartPopup.cpp
    src/ui/SearchView.cpp
    src/ui/PrivilegedRunner.cpp
    src/ui/UpdateManager.cpp
    src/ui/ProfileView.cpp
    src/utils/Config.cpp
)

# Header files
set(HEADERS
    src/core/PackageManager.h
    src/core/AURClient.h
    src/core/Database.h
    src/core/PacmanConfig.h
    src/core/ProfileManager.h
    src/models/Package.h
    src/models/PackageListModel.h
    src/ui/MainWindow.h
    src/ui/PackageView.h
    src/ui/AnalyticsView.h
    src/ui/ControlPanel.h
    src/ui/LoadingOverlay.h
    src/ui/ChartPopup.h
    src/ui/SearchView.h
    src/ui/PrivilegedRunner.h
    src/ui/UpdateManager.h
    src/ui/ProfileView.h
    src/utils/Config.h
)

# Resources
set(RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::Charts
    Qt6::Svg
    ${ALPM_LIBRARIES}
    ${CURL_LIBRARIES}
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES resources/archmaster.desktop DESTINATION share/applications)
install(FILES resources/icons/archmaster.svg DESTINATION share/icons/hicolor/scalable/apps)
