cmake_minimum_required(VERSION 3.21)
project(SPE1KFAControl VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets SerialPort Network)

set(SOURCES
    src/main.cpp
    src/protocol/StatusPacket.cpp
    src/connection/AbstractConnection.cpp
    src/connection/SerialConnection.cpp
    src/connection/TcpConnection.cpp
    src/controller/AmplifierController.cpp
    src/logging/TelemetryLogger.cpp
    src/ui/MainWindow.cpp
    src/ui/ConnectionDialog.cpp
    src/ui/AlarmHistoryDialog.cpp
    src/ui/widgets/MeterWidget.cpp
    src/ui/widgets/LedIndicator.cpp
)

qt_add_executable(SPE1KFAControl ${SOURCES})

target_include_directories(SPE1KFAControl PRIVATE src)

target_link_libraries(SPE1KFAControl PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Network
)

set_target_properties(SPE1KFAControl PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER com.spe.1kfa-control
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_BUNDLE_NAME "SPE 1K-FA Control"
)
