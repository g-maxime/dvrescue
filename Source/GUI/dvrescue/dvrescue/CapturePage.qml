import QtQuick 2.12
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.12
import ConnectionUtils 1.0

Rectangle {
    id: rectangle
    color: "#2e3436"

    width: 1190
    height: 768

    DecklinkConfigPopup {
        id: decklinkConfigPopup
        anchors.centerIn: parent

        property var rejectedCallback;
        property var acceptedCallback;

        onRejected: {
            console.debug('cancelled');

            if(rejectedCallback)
                rejectedCallback();
        }

        onAccepted: {
            console.debug('accepted');

            if(acceptedCallback)
                acceptedCallback();
        }
    }

    signal grabCompleted(string filePath);

    FunkyGridLayout {
        width: parent.width
        height: parent.height
        Repeater {
            id: captureViewRepeater
            model: devicesModel
            delegate: CaptureView {
                id: captureView
                property bool pendingAction: false;

                Timer {
                    repeat: true
                    running: deviceNameTextField.text !== ''
                    interval: 100
                    property bool querying: false
                    onTriggered: {
                        if(pendingAction)
                            return;

                        if(querying === false)
                        {
                            querying = true;
                            dvrescue.status(id).then((result) => {
                                console.debug('status: ', result.status)
                                if(pendingAction === false) {
                                    captureView.statusText = result.status
                                }
                                querying = false;
                            }).catch((err) => {
                                querying = false;
                            });
                        }
                    }
                }

                property int indexOfFramePos: -1;
                property int indexOfTimecode: -1;
                property int indexOfRecDateTime: -1;
                property int indexOfSourceSpeed: -1;
                property int indexOfFrameSpeed: -1;
                property int indexOfBlockErrors: -1;
                property int indexOfBlockErrorsEven: -1;

                property bool isDecklink: type === 'DeckLink'
                property int currentControlIndex: 0
                property int currentVideoModeIndex: 0
                property int currentVideoSourceIndex: 0
                property int currentAudioSourceIndex: 0
                property int currentTimecodesIndex: decklinkConfigPopup.timecodesModel.indexOf('vitc')

                function onColumnsChanged(columns) {

                    // ["FramePos","abst","abst_r","abst_nc","tc","tc_r","tc_nc","rdt","rdt_r","rdt_nc","rec_start","rec_end","Used","Status","Comments","BlockErrors","BlockErrors_Even","IssueFixed","SourceSpeed","FrameSpeed","InputPos","OutputPos"]
                    var columnNames = [];

                    columnNames = columns
                    console.debug('columnNames: ', JSON.stringify(columnNames))

                    indexOfFramePos = columnNames.indexOf('FramePos');
                    indexOfTimecode = columnNames.indexOf('tc');
                    indexOfRecDateTime = columnNames.indexOf('rdt');
                    indexOfSourceSpeed = columnNames.indexOf('SourceSpeed');
                    indexOfFrameSpeed = columnNames.indexOf('FrameSpeed');
                    indexOfBlockErrors = columnNames.indexOf('BlockErrors');
                    indexOfBlockErrorsEven = columnNames.indexOf('BlockErrors_Even');

                    console.debug('indexOfFramePos: ', indexOfFramePos)
                    console.debug('indexOfTimecode: ', indexOfTimecode)
                    console.debug('indexOfRecDateTime: ', indexOfRecDateTime)
                    console.debug('indexOfSourceSpeed: ', indexOfSourceSpeed)
                    console.debug('indexOfFrameSpeed: ', indexOfFrameSpeed)
                    console.debug('indexOfBlockErrors: ', indexOfBlockErrors)
                    console.debug('indexOfBlockErrorsEven: ', indexOfBlockErrorsEven)
                }

                function onEntriesReceived(entries) {
                    var framePos = 0;
                    if(indexOfFramePos !== -1) {
                        framePos = captureFrameInfo.frameNumber = entries[indexOfFramePos]
                    }

                    if(indexOfTimecode !== -1) {
                        captureFrameInfo.timeCode = entries[indexOfTimecode]
                    }

                    if(indexOfRecDateTime !== -1) {
                        var rdt = entries[indexOfRecDateTime];
                        captureFrameInfo.recTime = rdt;
                    }

                    /*
                    if(indexOfSourceSpeed !== -1) {
                        var sourceSpeed = entries[indexOfSourceSpeed]
                        speedValueText = sourceSpeed
                    }
                    */

                    if(indexOfFrameSpeed !== -1) {
                        var frameSpeedValue = entries[indexOfFrameSpeed]
                        if(frameSpeedValue)
                            frameSpeed = frameSpeedValue
                    }

                    if(indexOfBlockErrors !== -1 && indexOfBlockErrorsEven !== -1) {
                        var blockErrors = entries[indexOfBlockErrors]
                        var blockErrorsEven = entries[indexOfBlockErrorsEven]

                        dataModel.append(framePos, blockErrorsEven, blockErrors, captureView.capturingModeInt == captureView.playing)
                    }
                }

                function doCapture(captureCmd) {
                    csvParser.columnsChanged.disconnect(onColumnsChanged);
                    csvParserUI.entriesReceived.disconnect(onEntriesReceived);
                    ConnectionUtils.disconnect(csvParser, 'entriesReceived(const QStringList&)')
                    playbackBuffer.clear();

                    capturing = true;
                    pendingAction = true;
                    player.play()

                    statusText = "capturing..";
                    capturingMode = captureCmd;

                    var opts = [];
                    if(isDecklink) {
                        opts = makeDecklinkOptions();
                    }

                    dvrescue.capture(id, playbackBuffer, csvParser, captureCmd, opts, (launcher) => {
                       csvParser.columnsChanged.connect(onColumnsChanged);
                       var result = ConnectionUtils.connectToSignalQueued(csvParser, 'entriesReceived(const QStringList&)', csvParserUI, 'entriesReceived(const QStringList&)');
                       csvParserUI.entriesReceived.connect(onEntriesReceived);

                       console.debug('logging start capture command')
                       commandsLogs.logCommand(launcher);
                    }).then((result) => {
                        capturing = false;
                        capturingMode = '';
                        pendingAction = false;
                        player.stop();
                        commandsLogs.logResult(result.outputText);
                        return result;
                    }).catch((e) => {
                        capturing = false;
                        capturingMode = '';
                        pendingAction = false
                        player.stop();
                        commandsLogs.logResult(e);
                    });
                }

                function doDeckControl(deckControlCmd, deckControlStatus) {
                    pendingAction = true;
                    capturingMode = deckControlCmd;

                    statusText = deckControlStatus + "..";

                    var opts = [];
                    if(isDecklink) {
                        opts = makeDecklinkOptions();
                    }

                    dvrescue.control(id, deckControlCmd, opts, (launcher) => {
                        commandsLogs.logCommand(launcher);
                    }).then((result) => {
                        statusText = deckControlCmd + ".";
                        pendingAction = false;
                        commandsLogs.logResult(result.outputText);
                        return result;
                    });
                }

                rewindButton.visible: !isDecklink || currentControlIndex !== 0
                rewindButton.onClicked: {
                    if(!capturing)
                        doCapture('rew')
                    else
                        doDeckControl('rew', 'rewinding')
                }

                stopButton.visible: !isDecklink || currentControlIndex !== 0
                stopButton.onClicked: {
                    doDeckControl('stop', 'stopping')
                }

                rplayButton.visible: !isDecklink || currentControlIndex !== 0
                rplayButton.onClicked: {
                    if(!capturing)
                        doCapture('srew')
                    else
                        doDeckControl('srew', 'rplaying')
                }

                playButton.visible: !isDecklink || currentControlIndex !== 0
                playButton.onClicked: {
                    if(!capturing)
                        doCapture('play')
                    else
                        doDeckControl('play', 'playing')
                }

                fastForwardButton.visible: !isDecklink || currentControlIndex !== 0
                fastForwardButton.onClicked: {
                    if(!capturing)
                        doCapture('ff')
                    else
                        doDeckControl('ff', 'fast-forwarding')
                }

                function makeDecklinkOptions() {
                    var opts = []
                    if(currentControlIndex !== 0) {
                        opts = opts.concat(['--control', decklinkConfigPopup.controlsModel[currentControlIndex].id])
                    }

                    opts = opts.concat(
                        ['--decklink-video-mode', decklinkConfigPopup.videoModesModel[currentVideoModeIndex]],
                        ['--decklink-video-source', decklinkConfigPopup.videoSourcesModel[currentVideoSourceIndex]],
                        ['--decklink-audio-source', decklinkConfigPopup.audioSourcesModel[currentAudioSourceIndex]],
                        ['--decklink-timecode-format', decklinkConfigPopup.timecodesModel[currentTimecodesIndex]],
                    )
                    return opts;
                }

                captureButton.onClicked: {
                    specifyPathDialog.extension = isDecklink ? '.mkv' : '.dv'
                    specifyPathDialog.callback = (fileUrl) => {
                        csvParser.columnsChanged.disconnect(onColumnsChanged);
                        csvParserUI.entriesReceived.disconnect(onEntriesReceived);
                        ConnectionUtils.disconnect(csvParser, 'entriesReceived(const QStringList&)')
                        playbackBuffer.clear();

                        var filePath = urlToPath(fileUrl);

                        pendingAction = true;
                        player.play()

                        fileWriter.fileName = filePath;
                        fileWriter.open();

                        var columnNames = [];
                        var indexOfFramePos = -1;
                        var indexOfTimecode = -1;
                        var indexOfRecDateTime = -1;

                        grabbing = true;

                        var opts = [];
                        if(isDecklink) {
                            opts = makeDecklinkOptions();
                        }

                        dvrescue.grab(id, filePath, playbackBuffer, fileWriter, csvParser, opts, (launcher) => {
                           outputFilePath = filePath
                           csvParser.columnsChanged.connect(onColumnsChanged);
                           var result = ConnectionUtils.connectToSignalQueued(csvParser, 'entriesReceived(const QStringList&)', csvParserUI, 'entriesReceived(const QStringList&)');
                           csvParserUI.entriesReceived.connect(onEntriesReceived);

                           console.debug('logging grab command')
                           commandsLogs.logCommand(launcher);
                        }).then((result) => {
                           grabbing = false;
                           pendingAction = false;
                           player.stop();
                           commandsLogs.logResult(result.outputText);
                           grabCompleted(filePath)
                           return result;
                        }).catch((e) => {
                           grabbing = false;
                           pendingAction = false;
                           player.stop();
                           commandsLogs.logResult(e);
                        });
                    }

                    if(capturing) {
                        dvrescue.control(id, 'stop', (launcher) => {
                           commandsLogs.logCommand(launcher);
                        }).then((result) => {
                           statusText = "stopping.";
                           commandsLogs.logResult(result.outputText);

                           specifyPathDialog.reset();
                           specifyPathDialog.open();
                        });
                    } else {
                        specifyPathDialog.reset();
                        specifyPathDialog.open();
                    }
                }

                decklinkConfigButton.visible: isDecklink
                decklinkConfigButton.onClicked: {
                    var combinedControls = [{name : 'No control', id: ''}].concat(JSON.parse(controls))
                    console.debug(JSON.stringify(combinedControls))

                    decklinkConfigPopup.controlsModel = combinedControls;
                    decklinkConfigPopup.rejectedCallback = () => {
                        decklinkConfigPopup.currentControlIndex = currentControlIndex;
                        decklinkConfigPopup.currentVideoModeIndex = currentVideoModeIndex;
                        decklinkConfigPopup.currentVideoSourceIndex = currentVideoSourceIndex;
                        decklinkConfigPopup.currentAudioSourceIndex = currentAudioSourceIndex;
                        decklinkConfigPopup.currentTimecodesIndex = currentTimecodesIndex;

                        console.debug('currentControlIndex =', decklinkConfigPopup.currentControlIndex);
                        console.debug('currentVideoModeIndex =', decklinkConfigPopup.currentVideoModeIndex);
                        console.debug('currentVideoSourceIndex =', decklinkConfigPopup.currentVideoSourceIndex);
                        console.debug('currentAudioSourceIndex =', decklinkConfigPopup.currentAudioSourceIndex);
                        console.debug('currentTimecodesIndex =', decklinkConfigPopup.currentTimecodesIndex);
                    }

                    decklinkConfigPopup.acceptedCallback = () => {
                        currentControlIndex = decklinkConfigPopup.currentControlIndex;
                        currentVideoModeIndex = decklinkConfigPopup.currentVideoModeIndex;
                        currentVideoSourceIndex = decklinkConfigPopup.currentVideoSourceIndex;
                        currentAudioSourceIndex = decklinkConfigPopup.currentAudioSourceIndex;
                        currentTimecodesIndex = decklinkConfigPopup.currentTimecodesIndex;

                        console.debug('currentControlIndex =', currentControlIndex);
                        console.debug('currentVideoModeIndex =', currentVideoModeIndex);
                        console.debug('currentVideoSourceIndex =', currentVideoSourceIndex);
                        console.debug('currentAudioSourceIndex =', currentAudioSourceIndex);
                        console.debug('currentTimecodesIndex =', currentTimecodesIndex);
                    }

                    decklinkConfigPopup.currentControlIndex = currentControlIndex;
                    decklinkConfigPopup.currentVideoModeIndex = currentVideoModeIndex;
                    decklinkConfigPopup.currentVideoSourceIndex = currentVideoSourceIndex;
                    decklinkConfigPopup.currentAudioSourceIndex = currentAudioSourceIndex;
                    decklinkConfigPopup.currentTimecodesIndex = currentTimecodesIndex;

                    decklinkConfigPopup.open();
                }

                deviceNameTextField.text: {
                    console.debug('resolving device info for device: ', index)
                    return devicesModel.count === 0 ? '' : devicesModel.get(index).name + " (" + devicesModel.get(index).type + ")"
                }
            }
        }
    }

    AnimatedImage {
        anchors.centerIn: parent
        source: "/icons/no_decks.gif"
        playing: devicesModel.count === 0
        visible: devicesModel.count === 0
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:0.9}D{i:24}
}
##^##*/

