import qbs.FileInfo

QtApplication {
    Depends { name: "Qt.widgets" }
    win32.rc: "resource/app_icon.rc"
    cpp.defines: [
        // You can make your code fail to compile if it uses deprecated APIs.
        // In order to do so, uncomment the following line.
        //"QT_DISABLE_DEPRECATED_BEFORE=0x060000" // disables all the APIs deprecated before Qt 6.0.0
    ]
    cpp.includePaths: [
        "src/"
    ]

    cpp.cxxLanguageVersion: "c++14"

    files: [
        "README.md",
        "resource/icons.qrc",
        "resource/app_icon.rc",
        "src/*.cpp",
        "src/*.h",
        "src/*.ui"
    ]

    install: true
    installDir: qbs.targetOS.contains("qnx") ? FileInfo.joinPaths("/tmp", name, "bin") : base
    consoleApplication: false
}
