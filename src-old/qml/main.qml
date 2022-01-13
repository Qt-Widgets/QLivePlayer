import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Styles 1.4

Rectangle {
    id: root
    //    implicitWidth: 500
    //    implicitHeight: 700

    MHeader {
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        id: header
    }
    RowLayout {
        id: rowlayout
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        MTabBar {
            id: tbar
            Layout.fillHeight: true
            Layout.preferredWidth: 50
            //            Layout.Center:
        }

        SwipeView {
            id: swipeview
            clip: true
            interactive: false
            currentIndex: tbar.page_index
            orientation: Qt.Vertical
            Layout.fillWidth: true
            Layout.fillHeight: true
//            onCurrentIndexChanged: {
//                if (tbar.page_index_in !== currentIndex) {
//                    tbar.page_index_in = currentIndex
//                }
//            }

            Item {
                id: page1
                SavedPage {
                    id: savedPage
                    anchors.fill: parent
                }
            }

            Item {
                id: page2
                HistoryPage {
                    id: historyPage
                    anchors.fill: parent
                }
            }
        }
    }
}
