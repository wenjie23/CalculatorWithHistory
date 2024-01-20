import qbs.FileInfo

QtApplication {
    Depends { name: "Qt.widgets" }

    cpp.defines: [
        // You can make your code fail to compile if it uses deprecated APIs.
        // In order to do so, uncomment the following line.
        //"QT_DISABLE_DEPRECATED_BEFORE=0x060000" // disables all the APIs deprecated before Qt 6.0.0
    ]

    files: [
        "README.md",
        "display.cpp",
        "display.h",
        "icons.qrc",
        "main.cpp",
        "main_window.cpp",
        "main_window.h",
        "main_window.ui",
        "math_elements.cpp",
        "math_elements.h",
        "menu.cpp",
        "menu.h",
        "menu.ui",
    ]

    install: true
    installDir: qbs.targetOS.contains("qnx") ? FileInfo.joinPaths("/tmp", name, "bin") : base
    consoleApplication: false
}
