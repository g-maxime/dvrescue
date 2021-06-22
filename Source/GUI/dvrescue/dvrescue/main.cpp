#include "fileutils.h"
#include "settingsutils.h"
#include "qtavplayerutils.h"
#include "launcher.h"
#include "sortfiltertablemodel.h"
#include "qqmltablemodel_p.h"
#include "qqmltablemodelcolumn_p.h"
#include <datamodel.h>
#include <mediainfo.h>
#include <logging.h>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <qwtquick2plot.h>
#include <QQmlParserStatus>
#include <QQuickStyle>
#include <QtAV>

#ifdef STATIC
    Q_IMPORT_PLUGIN(QtAVQmlPlugin);
#endif

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    qmlRegisterType<Launcher>("Launcher", 0, 1, "Launcher");
    qmlRegisterType<DataModel>("DataModel", 1, 0, "DataModel");
    qmlRegisterType<MediaInfo>("MediaInfo", 1, 0, "MediaInfo");
    qmlRegisterType<QwtQuick2Plot>("QwtQuick2", 1, 0, "QwtQuick2Plot");
    qmlRegisterType<QwtQuick2PlotCurve>("QwtQuick2", 1, 0, "QwtQuick2PlotCurve");
    qmlRegisterType<QwtQuick2PlotGrid>("QwtQuick2", 1, 0, "QwtQuick2PlotGrid");
    qmlRegisterType<QwtQuick2PlotPicker>("QwtQuick2", 1, 0, "QwtQuick2PlotPicker");
    qmlRegisterType<QwtQuick2PlotLegend>("QwtQuick2", 1, 0, "QwtQuick2PlotLegend");
    qmlRegisterType<SortFilterTableModel>("SortFilterTableModel", 1, 0, "SortFilterTableModel");
    qmlRegisterType<QQmlTableModel>("TableModel", 1, 0, "TableModel");
    qmlRegisterType<QQmlTableModelColumn>("TableModelColumn", 1, 0, "TableModelColumn");

    qRegisterMetaType<MarkerInfo>();
    qRegisterMetaType<QList<MarkerInfo>>();
    qRegisterMetaType<QAbstractTableModel*>();
    qRegisterMetaType<QProcess::ProcessState>();
    qRegisterMetaType<QProcess::ExitStatus>();
    qRegisterMetaType<QProcess::ProcessError>();

    Logging logging;

    auto version = QtAV_Version_String();

    QApplication app(argc, argv);

#ifdef Q_OS_WIN
    auto appDirPath = QCoreApplication::applicationDirPath();
    qDebug() << "appDirPath: " << appDirPath;
    auto paths = QProcessEnvironment::systemEnvironment().value("PATH");
    auto additionalPath =
            QDir::toNativeSeparators(appDirPath + "/" + "cygwin/bin") + ";" +
            QDir::toNativeSeparators(appDirPath + "/" + "scripts") + ";" +
            QDir::toNativeSeparators(appDirPath + "/" + "tools") + ";";

    paths.prepend(additionalPath);
    qputenv("PATH", paths.toUtf8());
#endif //

    app.setOrganizationName("dvrescue");
    app.setOrganizationDomain("dvrescue.com");
    app.setApplicationName("dvrescue");

    QCommandLineParser parser;
    parser.setApplicationDescription("dvrescue: GUI");

    QCommandLineOption resetSettingsOption(QStringList() << "r" << "resetsettings", "reset application settings");
    parser.addOption(resetSettingsOption);

    QCommandLineOption enableLoggingOption(QStringList() << "l" << "log", "enable file logging");
    parser.addOption(enableLoggingOption);

    parser.process(app.arguments());
    if(parser.isSet(resetSettingsOption)) {
        qDebug() << "resetting settings...";
        QSettings settings;
        auto allKeys = settings.allKeys();
        for(auto& key : allKeys) {
            settings.remove(key);
        }
    }
    if(parser.isSet(enableLoggingOption)) {
        qDebug() << "enable file logging...";
        logging.enable();
    }

    qDebug() << "PATH: " << qEnvironmentVariable("PATH");
    QQuickStyle::setStyle("Material");

    qmlRegisterSingletonType<FileUtils>("FileUtils", 1, 0, "FileUtils", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)

        FileUtils *utils = new FileUtils();
        return utils;
    });

    qmlRegisterSingletonType<FileUtils>("SettingsUtils", 1, 0, "SettingsUtils", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)

        SettingsUtils *utils = new SettingsUtils();
        return utils;
    });

    qmlRegisterSingletonType<QtAVPlayerUtils>("QtAVPlayerUtils", 1, 0, "QtAVPlayerUtils", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)

        QtAVPlayerUtils *utils = new QtAVPlayerUtils();
        return utils;
    });

    QQmlApplicationEngine engine;
    DataModel::setEngine(&engine);

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
