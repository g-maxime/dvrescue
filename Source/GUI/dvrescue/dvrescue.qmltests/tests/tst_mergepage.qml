import QtQuick 2.0
import QtTest 1.0
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Qt.labs.settings 1.0
import "qrc:/"

Rectangle {
    color: "#2e3436"
    width: Math.max(1200, parent ? parent.width : 0)
    height: Math.max(800, parent ? parent.height : 0)

    DvRescueReport {
        id: report
    }

    Settings {
        id: settings;

        property string endTheCaptureIftheTapeContainsNoDataFor
        property bool saveALogOfTheCaptureProcess

        property bool advancedFrameTable
        property var frameTableColumns: []

        property string dvrescueCmd
        property string xmlStarletCmd
        property string mediaInfoCmd
        property string ffmpegCmd
        property bool debugVisible

        Component.onCompleted: {
            console.debug('settings initialized')
        }
    }

    DvRescueCLI {
        id: dvrescue
        cmd: settings.dvrescueCmd

        Component.onCompleted: {
            console.debug('dvrescuecli completed...');
        }
    }

    MergePage {
        id: mergePage
        anchors.fill: parent
        visible: false;

        filesModel: filesModel
    }

    FilesModel {
        id: filesModel
        mediaInfoModel: instantiator

        onAppended: {
            var index = filesModel.count - 1
            console.debug('FilesModel: onAppended', index, JSON.stringify(fileInfo))
        }
    }

    MediaInfoModel {
        id: instantiator
        model: filesModel
        onObjectAdded: {
            console.debug('MediaInfoModel: added', index, JSON.stringify(object))
        }
        onObjectRemoved: {
            console.debug('MediaInfoModel: removed', index, JSON.stringify(object))
        }
    }

    TestCase {
        name: "TestMergePage"
        when: windowShown
        id: testcase

        function initTestCase() {
        }

        function test_mergePage() {
            mergePage.visible = true;

            filesModel.add('D:\\Projects\\dvrescue-work\\videos\\BAVC1010689_take01_12M.dv');
            filesModel.add('D:\\Projects\\dvrescue-work\\videos\\BAVC1010689_take02_12M.dv');

            /*
            var data = 'FramePos,abst,abst_r,abst_nc,tc,tc_r,tc_nc,rdt,rdt_r,rdt_nc,rec_start,rec_end,Used,Status,Comments,BlockErrors,BlockErrors_Even,IssueFixed,SourceSpeed,FrameSpeed,InputPos,OutputPos
0,30,,,00:00:00;03,,,2020-08-29 14:55:03,,,,,0, M,,0,0,0,0.0,31,0|,0
1,40,,,00:00:00;04,,,2020-08-29 14:55:03,,,,,0,,,0,0,0,0.0,31,120000|0,120000
2,50,,,00:00:00;05,,,2020-08-29 14:55:03,,,,,0,,,0,0,0,0.0,31,240000|120000,240000
3,60,,,00:00:00;06,,,2020-08-29 14:55:03,,,,,0,,,0,0,0,0.0,31,360000|240000,360000
4,70,,,00:00:00;07,,,2020-08-29 14:55:03,,,,,0,,,0,0,0,0.0,31,480000|360000,480000
5,80,,,00:00:00;08,,,2020-08-29 14:55:03,,,,,0,,,0,0,0,0.0,31,600000|480000,600000
6,90,,,00:00:00;09,,,2020-08-29 14:55:03,,,,,0,,,0,0,0,0.0,31,720000|600000,720000
7,100,,,00:00:00;10,,,2020-08-29 14:55:03,,,,,0,,,0,0,0,0.0,31,840000|720000,840000
8,110,,,00:00:00;11,,,2020-08-29 14:55:03,,,,,0,,,0,0,0,0.0,31,960000|840000,960000'

            var csvParser = mergePage.csvParser
            var mergeReportView = mergePage.mergeReportView

            data = data.split('\n').join('\r\n')
            csvParser.rows = [];
            csvParser.write(data);

            mergeReportView.refresh();
            */
            wait(1000000)
        }

        function cleanupTestCase() {
        }
    }
}
