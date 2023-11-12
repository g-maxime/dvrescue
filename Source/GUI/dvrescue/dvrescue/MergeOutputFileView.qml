import QtQuick 2.0
import QtQuick 2.12
import TableModel 1.0
import TableModelColumn 1.0
import SortFilterTableModel 1.0
import QtQuick.Controls 2.12
import QtQml 2.12
import Qt.labs.qmlmodels 1.0
import MediaInfo 1.0
import FileUtils 1.0

Rectangle {
    color: 'transparent'

    signal deleteClicked(var row);

    property alias dataModel: dataModel
    property alias tableView: tableView

    readonly property string filePathColumn: "Output File Path"
    readonly property string statusColumn: "Status"
    readonly property string errorColumn: "Error"

    function forceLayout() {
        tableView.forceLayout();
    }

    onWidthChanged: {
        tableView.forceLayout();
    }
    onHeightChanged: {
        tableView.forceLayout();
    }

    function newRow(path) {
        var rowEntry = {}
        rowEntry[filePathColumn] = path
        rowEntry[statusColumn] = "not ready"
        rowEntry[errorColumn] = ""

        dataModel.appendRow(rowEntry)
    }

    function scrollToTop() {
        tableView.view.contentY = -16
    }

    Text {
        id: label
        text: 'MERGE RESULTS'
        color: 'white'
        font.pixelSize: 20
    }

    Rectangle {
        color: 'white'
        anchors.top: label.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        TableViewEx {
            id: tableView
            anchors.fill: parent
            anchors.margins: 1
            model: sortFilterTableModel
            dataModel: dataModel
            delegateHeight: 25

            Menu {
                id: contextMenu
                property int currentRow: -1

                MenuItem {
                    text: "Show location"
                    onClicked: {
                        var filePath = dataModel.getRow(contextMenu.currentRow)[filePathColumn];
                        Qt.openUrlExternally('file:///' + FileUtils.getFileDir(filePath))
                    }
                }

                function showBelowControl(control) {
                    var mapped = control.mapToItem(tableView, 0, 0);
                    contextMenu.x = mapped.x
                    contextMenu.y = mapped.y + control.height

                    contextMenu.open()
                }

                function show(pos, row) {
                    contextMenu.x = pos.x
                    contextMenu.y = pos.y
                    currentRow = row

                    contextMenu.open()
                }
            }

            headerDelegate: SortableFiltrableColumnHeading {
                id: header
                width: tableView.columnWidths[modelData] ? tableView.columnWidths[modelData] : 50
                text: dataModel.columns[modelData].display
                canFilter: false
                canSort: false
                canShowIndicator: false
                filterFont.pixelSize: 11
                textFont.pixelSize: 13
                height: tableView.columnWidths, tableView.getMaxDesiredHeight()

                onFilterTextChanged: {
                    sortFilterTableModel.setFilterText(modelData, filterText);
                }

                Rectangle {
                    id: handle
                    color: "transparent"
                    height: parent.height
                    width: 10
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    MouseArea {
                        id: mouseHandle
                        anchors.fill: parent
                        drag{ target: parent; axis: Drag.XAxis }
                        hoverEnabled: true
                        cursorShape: Qt.SizeHorCursor
                        onMouseXChanged: {
                            if (drag.active) {
                                var newWidth = header.width + mouseX
                                if (newWidth >= minimumWidth) {
                                    // header.width = newWidth

                                    var newWidths = tableView.columnWidths
                                    var oldWidth = newWidths[modelData]
                                    newWidths[modelData] = newWidth;

                                    tableView.columnWidths = newWidths
                                    tableView.view.contentWidth += newWidth - oldWidth
                                    tableView.forceLayout();
                                }
                            }
                        }
                    }
                }
            }

            delegate: DelegateChooser {
                DelegateChoice  {
                    column: 0

                    TextDelegate {
                        height: tableView.delegateHeight
                        implicitHeight: tableView.delegateHeight
                        property color evenColor: '#e3e3e3'
                        property color oddColor: '#f3f3f3'
                        property color redColor: 'red'
                        textFont.pixelSize: 13
                        text: display

                        color: (row % 2) == 0 ? evenColor : oddColor
                        CustomButton {
                            id: deleteButton
                            anchors.left: parent.left
                            anchors.leftMargin: 2

                            icon.color: "black"
                            icon.source: "/icons/exit.svg"
                            implicitHeight: parent.height
                            implicitWidth: implicitHeight

                            onClicked: {
                                deleteClicked(row);
                            }
                        }

                        MouseArea {
                            acceptedButtons: Qt.RightButton
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                if(mouse.button == Qt.RightButton) {
                                    if(dataModel.getRow(row)[statusColumn] === 'finished')
                                        contextMenu.show(mapToItem(tableView, mouseX, mouseY), row);
                                }
                            }
                        }
                    }
                }

                DelegateChoice  {
                    Rectangle {
                        id: statusDelegate
                        implicitWidth: 100
                        implicitHeight: 20
                        property color evenColor: '#e3e3e3'
                        property color oddColor: '#f3f3f3'

                        color: (row % 2) == 0 ? evenColor : oddColor
                        property real overlayColorOpacity: 0.5
                        property alias overlayColor: overlay.color
                        property alias overlayVisible: overlay.visible

                        Image {
                            source: "/icons/dvpackager_processing-not-started.svg"
                            height: statusDelegate.height
                            width: height
                            anchors.centerIn: parent
                            visible: display == 'not exported'
                        }

                        Image {
                            source: "/icons/dvpackager_processing-in-queue.svg"
                            height: statusDelegate.height
                            width: height
                            anchors.centerIn: parent
                            visible: display == 'queued'
                        }

                        Image {
                            source: "/icons/dvpackager_processing-in-process.svg"
                            height: statusDelegate.height
                            width: height
                            anchors.centerIn: parent
                            visible: display == 'packaging'

                            RotationAnimation on rotation {
                                loops: Animation.Infinite
                                from: 0
                                to: 360
                                duration: 2000
                            }
                        }

                        Image {
                            source: "/icons/dvpackager_processing-done.svg"
                            height: statusDelegate.height
                            width: height
                            anchors.centerIn: parent
                            visible: display == 'finished' && edit == ''
                        }

                        Image {
                            source: "/icons/dvpackager_processing-failed-canceled.svg"
                            height: statusDelegate.height
                            width: height
                            anchors.centerIn: parent
                            visible: display == 'finished' && edit != ''
                        }

                        Rectangle {
                            id: overlay
                            anchors.fill: parent
                            opacity: overlayColorOpacity
                            visible: false
                        }
                    }
                }
            }
        }
    }

    SortFilterTableModel {
        id: sortFilterTableModel
        tableModel: dataModel
    }

    property int columnSpacing: 10

    TableModelEx {
        id: dataModel

        TableModelColumn {
            display: "Output File Path"
            property int minWidth: 250
        }

        TableModelColumn {
            display: "Status"
            edit: "Error"
            property int minWidth: 40
        }
    }

    function updatePackagingErrorByPath(path, error) {
        console.debug('PackageOutputFileView: updatePackagingErrorByPath: ', path, error);
        updatePropertyByPath(path, 'Error', error)
    }

    function updatePackagingStatusByPath(path, status) {
        console.debug('PackageOutputFileView: updatePackagingStatusByPath: ', path, status);
        updatePropertyByPath(path, 'Status', status)
    }

    function updatePackagingError(index, error) {
        console.debug('PackageOutputFileView: updatePackagingError: ', index, error);
        updateProperty(index, 'Error', error)
    }

    function updatePackagingStatus(index, status) {
        console.debug('PackageOutputFileView: updatePackagingStatus: ', index, status);
        updateProperty(index, 'Status', status)
    }

    function updatePropertyByPath(path, propertyName, value) {
        for(var j = 0; j < dataModel.rowCount; ++j) {
            var row = dataModel.getRow(j)
            if((row['Output File Path']) === path) {
                updateProperty(j, propertyName, value)
                break;
            }
        }
    }

    function updateProperty(index, propertyName, value) {
        var rowData = dataModel.getRow(index)
        var newRowData = JSON.parse(JSON.stringify(rowData))
        newRowData[propertyName] = value
        dataModel.setRow(index, newRowData)
    }
}
