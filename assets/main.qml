import bb.cascades 1.4
import bb.cascades.pickers 1.0
import bb.data 1.0

NavigationPane {
    id: nav
    Page {
        titleBar: TitleBar {
            kind: TitleBarKind.FreeForm
            kindProperties: FreeFormTitleBarKindProperties {
                Container {
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    leftPadding: ui.sddu(1)
                    rightPadding: leftPadding
                    minHeight: 75
                    TextField {
                        id: username_at_hostname_textfield
                        text: {
                            if (_downloader.username != "" && _downloader.hostname != "") {
                                text = _downloader.username + "@" + _downloader.hostname;
                            }
                        }
                        hintText: qsTr("usuário@servidor:porta")
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 5
                        }
                        verticalAlignment: VerticalAlignment.Center
                        maximumLength: 30
                    }
                    TextField {
                        id: password_textfield
                        text: _downloader.password
                        hintText: qsTr("Senha")
                        inputMode: TextFieldInputMode.Password
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 3
                        }
                        verticalAlignment: VerticalAlignment.Center
                    }
                }
            }
        }
        actions: [
            ActionItem {
                title: qsTr("Enviar")
                enabled: _downloader.connectLabel == "Parar"
                imageSource: "images/ic_share.png"
                ActionBar.placement: ActionBarPlacement.OnBar
                onTriggered: {
                    file_picker.open();
                }
            },
            ActionItem {
            title: _downloader.connectLabel
            enabled: _downloader.connectPossible
            imageSource: "images/ic_reload.png"
            ActionBar.placement: ActionBarPlacement.Signature
            onTriggered: {
            var username_at_hostname = username_at_hostname_textfield.text;
              var index = username_at_hostname.indexOf("@");
              if (index != -1) {
                 var username = username_at_hostname.substring(0, index);
                 var hostname = username_at_hostname.substring(index + 1);
                 if (username != "" && hostname != "") {
                    _downloader.hostname = hostname;
                    _downloader.username = username;
                    _downloader.password = password_textfield.text;
                    _downloader.connectOrDisconnect();
                    return;
                    }
                 }
                
                    _downloader.statusText = qsTr("Preencha corretamente os campos necessários.");
                }
                
            },
            ActionItem {
                title: qsTr("Baixar")
                enabled: _downloader.downloadPossible
                imageSource: "images/ic_reply.png"
                ActionBar.placement: ActionBarPlacement.OnBar
                onTriggered: {
                    _downloader.downloadFile();
                    listview.clearSelection();
                }
            }
        ]
        content:
            
        Container {
            layout: DockLayout {
            }
            
        Container {
             ListView {
                id: listview
                dataModel: _model
                listItemProvider: _itemProvider
                enabled: _downloader.selectionPossible
                scrollRole: ScrollRole.Main
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                topMargin: ui.sddu(1)
                onTriggered: {
                    clearSelection();
                    if (! _downloader.isDirectory(indexPath)) {
                        select(indexPath);
                    }
                    _downloader.processItem(indexPath);
                }
            }
         }
        ActivityIndicator {
            id: wait_indicator
            preferredWidth: 180
            preferredHeight: 180
            running: _downloader.waitIndicatorPossible
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center
        }
        
        attachedObjects: [
            FilePicker {
                id: file_picker
                type: defaultType
                title: qsTr("Selecionar o arquivo")
                directories: [ "/accounts/1000/shared/" ]
                onFileSelected: {
                    _downloader.uploadFile(selectedFiles);
                }
            }
        ]
        
    } } }